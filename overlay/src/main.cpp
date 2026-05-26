// Copyright (c) 2024 averne / 2025 Dimasick
//
// This file is part of Fizeau (Ryazhenka rework).
//
// Fizeau is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

#define NDEBUG
#define STBTT_STATIC
#define TESLA_INIT_IMPL

#include <exception_wrap.hpp>
#include <tesla.hpp>
#include <common.hpp>
#include "i18n.hpp"
#include "presets.hpp"

#ifdef DEBUG
TwiliPipe g_twlPipe;
#endif

namespace fz {

namespace {

template <typename ...Args>
std::string format(const std::string_view &fmt, Args &&...args) {
    std::string str(std::snprintf(nullptr, 0, fmt.data(), args...) + 1, 0);
    std::snprintf(str.data(), str.capacity(), fmt.data(), args...);
    return str;
}

bool is_full(const ColorRange &range) {
    return (range.lo == MIN_RANGE) && (range.hi == MAX_RANGE);
}

} // namespace


// ========================================
// ServiceInactiveGui
// ========================================
class ServiceInactiveGui: public tsl::Gui {
public:
    ServiceInactiveGui() { }

    virtual ~ServiceInactiveGui() {
        tsl::Overlay::get()->close();
    }

    virtual tsl::elm::Element *createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("Fizeau", VERSION, false);

        auto* drawer = new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(str::SERVICE_NOT_ACTIVE_1(), false, x + 16, y +  80, 20, (0xffff));
            renderer->drawString(str::SERVICE_NOT_ACTIVE_2(), false, x + 16, y + 110, 20, (0xffff));
            renderer->drawString(str::SERVICE_NOT_ACTIVE_3(), false, x + 16, y + 130, 20, (0xffff));
        });

        frame->setContent(drawer);

        #if USING_WIDGET_DIRECTIVE
        frame->m_showWidget = true;
        #endif

        return frame;
    }
};


// ========================================
// ErrorGui
// ========================================
class ErrorGui: public tsl::Gui {
public:
    ErrorGui(Result rc): rc(rc) { }

    virtual ~ErrorGui() {
        tsl::Overlay::get()->close();
    }

    virtual tsl::elm::Element *createUI() override {
        auto* frame = new tsl::elm::OverlayFrame("Fizeau", VERSION, false);

        auto* drawer = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(format("%#x (%04d-%04d)", this->rc, R_MODULE(this->rc) + 2000, R_DESCRIPTION(this->rc)).c_str(),
                                                                         false, x, y +  50, 20, (0xffff));
            renderer->drawString(str::ERROR_OCCURRED(),     false, x, y +  80, 20, (0xffff));
            renderer->drawString(str::ERROR_USE_LATEST_1(), false, x, y + 110, 20, (0xffff));
            renderer->drawString(str::ERROR_USE_LATEST_2(), false, x, y + 130, 20, (0xffff));
            renderer->drawString(str::ERROR_REPORT(),       false, x, y + 150, 20, (0xffff));
            renderer->drawString("https://github.com/Dimasick-git/Fizeau", false, x, y + 170, 18, (0xffff));
        });

        frame->setContent(drawer);

        #if USING_WIDGET_DIRECTIVE
        frame->m_showWidget = true;
        #endif

        return frame;
    }

private:
    Result rc;
};


// ========================================
// FizeauOverlayGui — forward declaration (needed by PresetsGui)
// ========================================
class FizeauOverlayGui;


// ========================================
// PresetsGui — preset selection screen
// ========================================
class PresetsGui: public tsl::Gui {
public:
    PresetsGui(FizeauSettings *day_s, FizeauSettings *night_s, bool *is_day_flag, Config *cfg)
        : day_settings(day_s), night_settings(night_s), is_day(is_day_flag), config(cfg) {
        g_presets.load();
    }

    virtual tsl::elm::Element *createUI() override {
        auto* frame = new tsl::elm::OverlayFrame(str::PRESETS_TITLE(), VERSION);
        auto* list  = new tsl::elm::List();

        // Save current settings as new preset
        auto* save_btn = new tsl::elm::ListItem(str::SAVE_CURRENT());
        save_btn->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_A) {
                auto &ds = *this->day_settings;
                g_presets.saveCurrentAs(ds.temperature, ds.saturation,
                                        ds.hue, ds.contrast, ds.gamma, ds.luminance);
                tsl::changeTo<PresetsGui>(this->day_settings, this->night_settings, this->is_day, this->config);
                return true;
            }
            return false;
        });
        list->addItem(save_btn);

        // Built-in presets
        list->addItem(new tsl::elm::CategoryHeader(str::BUILTIN_LABEL()));

        for (const auto &p : getBuiltinPresets()) {
            auto* btn = new tsl::elm::ListItem(p.name);
            btn->setValue(str::APPLY_PRESET());
            btn->setClickListener([this, p](std::uint64_t keys) {
                if (keys & HidNpadButton_A) {
                    this->apply_preset(p);
                    tsl::goBack();
                    return true;
                }
                return false;
            });
            list->addItem(btn);
        }

        // Custom presets
        if (!g_presets.custom.empty()) {
            list->addItem(new tsl::elm::CategoryHeader(str::CUSTOM_LABEL()));
            for (std::size_t i = 0; i < g_presets.custom.size(); ++i) {
                const auto &p = g_presets.custom[i];
                auto* btn = new tsl::elm::ListItem(p.name);
                btn->setValue(str::APPLY_DEL_PRESET());
                btn->setClickListener([this, i](std::uint64_t keys) {
                    if (keys & HidNpadButton_A) {
                        if (i < g_presets.custom.size())
                            this->apply_preset(g_presets.custom[i]);
                        tsl::goBack();
                        return true;
                    }
                    if (keys & HidNpadButton_X) {
                        g_presets.removeCustom(i);
                        tsl::changeTo<PresetsGui>(this->day_settings, this->night_settings, this->is_day, this->config);
                        return true;
                    }
                    return false;
                });
                list->addItem(btn);
            }
        }

        frame->setContent(list);
        return frame;
    }

private:
    FizeauSettings *day_settings;
    FizeauSettings *night_settings;
    bool           *is_day;
    Config         *config;

    void apply_preset(const FzPreset &p) {
        auto apply_to = [&](FizeauSettings &s) {
            s.temperature = p.temperature;
            s.saturation  = p.saturation;
            s.hue         = p.hue;
            s.contrast    = p.contrast;
            s.gamma       = p.gamma;
            s.luminance   = p.luminance;
        };
        apply_to(*this->day_settings);
        apply_to(*this->night_settings);
        this->config->apply();
    }
};


// ========================================
// FizeauOverlayGui
// ========================================

enum class PeriodOverride : std::uint8_t {
    Dynamic = 0,
    Day     = 1,
    Night   = 2,
};

static constexpr Time OVERRIDE_DAY_DUSK_BEGIN = {23, 59, 0};
static constexpr Time OVERRIDE_DAY_DUSK_END   = {23, 59, 0};
static constexpr Time OVERRIDE_DAY_DAWN_BEGIN = {0,  0,  0};
static constexpr Time OVERRIDE_DAY_DAWN_END   = {0,  0,  0};

static constexpr Time OVERRIDE_NIGHT_DUSK_BEGIN = {0, 0, 0};
static constexpr Time OVERRIDE_NIGHT_DUSK_END   = {0, 0, 0};
static constexpr Time OVERRIDE_NIGHT_DAWN_BEGIN = {0, 0, 0};
static constexpr Time OVERRIDE_NIGHT_DAWN_END   = {0, 0, 0};

struct ProfilePeriodState {
    PeriodOverride override = PeriodOverride::Dynamic;
    Time real_dusk_begin = {};
    Time real_dusk_end   = {};
    Time real_dawn_begin = {};
    Time real_dawn_end   = {};
};

// ── TimeStepTrackBar ──────────────────────────────────────────────────────────
class TimeStepTrackBar : public tsl::elm::StepTrackBar {
public:
    static constexpr std::size_t kNumSteps = 25;
    static constexpr int         kMaxHour  = static_cast<int>(kNumSteps) - 1;

    explicit TimeStepTrackBar(const std::string &label)
        : tsl::elm::StepTrackBar("", kNumSteps, true, true, label) {
        this->TrackBar::m_numSteps = kNumSteps;
        this->m_selection = "00:00";
    }

    virtual void setProgress(u16 hour) override {
        tsl::elm::StepTrackBar::setProgress(hour);
        this->m_selection = hour_to_hhmm(hour);
    }

    void setValueChangedListener(std::function<void(u16)> listener) {
        tsl::elm::TrackBar::setValueChangedListener([this, listener](u16 hour) {
            this->m_selection = hour_to_hhmm(hour);
            listener(hour);
        });
    }

    static std::string hour_to_hhmm(int hour) {
        char buf[6];
        std::snprintf(buf, sizeof(buf), "%02d:00", std::clamp(hour, 0, kMaxHour));
        return buf;
    }

    static int time_to_hour(Time t) {
        return std::clamp((int)t.h, 0, kMaxHour);
    }

    static Time hour_to_time(int hour) {
        return { (std::uint8_t)std::clamp(hour, 0, kMaxHour), 0, 0 };
    }
};

// ── DynamicProfileTrackBar ────────────────────────────────────────────────────
class DynamicProfileTrackBar : public tsl::elm::NamedStepTrackBar {
public:
    DynamicProfileTrackBar(std::vector<std::string> labels, const std::string &title)
        : tsl::elm::NamedStepTrackBar("", { "" }, true, title, true) {
        this->m_stepDescriptions = std::move(labels);
        const u8 n = static_cast<u8>(this->m_stepDescriptions.size());
        this->m_numSteps           = n;
        this->TrackBar::m_numSteps = n;
        if (!this->m_stepDescriptions.empty())
            this->m_selection = this->m_stepDescriptions.front();
    }
};

class FizeauOverlayGui: public tsl::Gui {
public:
    FizeauOverlayGui(FizeauProfileId forced_profile = FizeauProfileId_Invalid)
        : allow_high_temp(false), apply_counter(0), pending_apply(false) {
        this->rc = fizeauInitialize();
        if (R_FAILED(rc))
            return;

        this->config.read();
        this->pad_config_to_four_profiles();
        this->num_profiles = count_profiles_in_config();

        if (this->config.internal_profile >= this->num_profiles)
            this->config.internal_profile = FizeauProfileId_Profile1;
        if (this->config.external_profile >= this->num_profiles)
            this->config.external_profile = FizeauProfileId_Profile1;

        if (this->rc = fizeauGetIsActive(&this->config.active); R_FAILED(this->rc))
            return;
        if (this->rc = apmGetPerformanceMode(&this->perf_mode); R_FAILED(this->rc))
            return;

        FizeauProfileId target_profile;
        if (forced_profile != FizeauProfileId_Invalid) {
            target_profile = forced_profile;
        } else {
            target_profile = (this->perf_mode == ApmPerformanceMode_Normal)
                ? config.internal_profile : config.external_profile;
        }

        if (this->rc = this->config.open_profile(target_profile); R_FAILED(this->rc))
            return;

        this->load_period_overrides();

        auto &ps = this->period_states[target_profile];
        if (ps.override == PeriodOverride::Dynamic) {
            ps.real_dusk_begin = this->config.profile.dusk_begin;
            ps.real_dusk_end   = this->config.profile.dusk_end;
            ps.real_dawn_begin = this->config.profile.dawn_begin;
            ps.real_dawn_end   = this->config.profile.dawn_end;
        }

        this->is_day = this->compute_is_day();
        this->allow_high_temp =
            (this->is_day ? this->config.profile.day_settings.temperature
                          : this->config.profile.night_settings.temperature) > D65_TEMP;
    }

    virtual ~FizeauOverlayGui() {
        if (this->pending_apply)
            this->config.apply();
        this->save_period_overrides();
        this->config.write();
        fizeauExit();
    }

    bool compute_is_day() const {
        auto id = this->config.cur_profile_id;
        if (id < FizeauProfileId_Total) {
            switch (this->period_states[id].override) {
                case PeriodOverride::Day:   return true;
                case PeriodOverride::Night: return false;
                default: break;
            }
        }
        return Clock::is_in_interval(this->config.profile.dawn_end, this->config.profile.dusk_begin);
    }

    void refresh_sliders() {
        this->is_day = this->compute_is_day();

        if (this->period_button) {
            auto id = this->config.cur_profile_id;
            auto ov = (id < FizeauProfileId_Total) ? this->period_states[id].override : PeriodOverride::Dynamic;
            this->period_button->setValue(
                (ov == PeriodOverride::Day)   ? str::MODE_DAY()   :
                (ov == PeriodOverride::Night) ? str::MODE_NIGHT() :
                                                str::MODE_DYNAMIC());
        }

        this->allow_high_temp =
            (this->is_day ? this->config.profile.day_settings.temperature
                          : this->config.profile.night_settings.temperature) > D65_TEMP;

        this->temp_slider->setProgress(
            ((this->is_day ? this->config.profile.day_settings.temperature
                           : this->config.profile.night_settings.temperature) - MIN_TEMP)
            * 100 / ((this->allow_high_temp ? MAX_TEMP : D65_TEMP) - MIN_TEMP));

        this->sat_slider->setProgress(
            ((this->is_day ? this->config.profile.day_settings.saturation
                           : this->config.profile.night_settings.saturation) - MIN_SAT)
            * 100 / (MAX_SAT - MIN_SAT));

        this->hue_slider->setProgress(
            ((this->is_day ? this->config.profile.day_settings.hue
                           : this->config.profile.night_settings.hue) - MIN_HUE)
            * 100 / (MAX_HUE - MIN_HUE));

        this->components_bar->setProgress(static_cast<u8>(this->config.profile.components));

        this->filter_bar->setProgress(
            (this->config.profile.filter == Component_None) ? 0
            : std::countr_zero(static_cast<std::uint32_t>(this->config.profile.filter)) + 1);

        this->contrast_slider->setProgress(
            ((this->is_day ? this->config.profile.day_settings.contrast
                           : this->config.profile.night_settings.contrast) - MIN_CONTRAST)
            * 100 / (MAX_CONTRAST - MIN_CONTRAST));

        this->gamma_slider->setProgress(
            ((this->is_day ? this->config.profile.day_settings.gamma
                           : this->config.profile.night_settings.gamma) - MIN_GAMMA)
            * 100 / (MAX_GAMMA - MIN_GAMMA));

        this->luma_slider->setProgress(
            ((this->is_day ? this->config.profile.day_settings.luminance
                           : this->config.profile.night_settings.luminance) - MIN_LUMA)
            * 100 / (MAX_LUMA - MIN_LUMA));

        auto &range = (this->is_day ? this->config.profile.day_settings.range
                                    : this->config.profile.night_settings.range);
        this->range_button->setValue(is_full(range) ? str::FULL() : str::LIMITED());

        if (this->dawn_slider && this->dusk_slider) {
            auto id = this->config.cur_profile_id;
            if (id < FizeauProfileId_Total) {
                auto &ps = this->period_states[id];
                this->dawn_slider->setProgress(TimeStepTrackBar::time_to_hour(ps.real_dawn_begin));
                this->dusk_slider->setProgress(TimeStepTrackBar::time_to_hour(ps.real_dusk_begin));
            }
        }
    }

    static constexpr const char *overrides_path = "/config/fizeau/period_overrides.ini";
    static constexpr const char *config_ini_path = "/config/fizeau/config.ini";

    void pad_config_to_four_profiles() {
        std::size_t existing = count_profiles_in_config();
        if (existing >= FizeauProfileId_Total)
            return;

        FILE *fp = std::fopen(config_ini_path, "a");
        if (fp) {
            for (std::size_t i = existing + 1; i <= FizeauProfileId_Total; ++i) {
                std::fprintf(fp, "\n[profile%zu]\n",  i);
                std::fprintf(fp, "dusk_begin        = 20:00\n");
                std::fprintf(fp, "dusk_end          = 20:00\n");
                std::fprintf(fp, "dawn_begin        = 07:00\n");
                std::fprintf(fp, "dawn_end          = 07:00\n");
                std::fprintf(fp, "temperature_day   = 6500\n");
                std::fprintf(fp, "temperature_night = 6500\n");
                std::fprintf(fp, "saturation_day    = 1.000000\n");
                std::fprintf(fp, "saturation_night  = 1.000000\n");
                std::fprintf(fp, "hue_day           = 0.000000\n");
                std::fprintf(fp, "hue_night         = 0.000000\n");
                std::fprintf(fp, "components        = all\n");
                std::fprintf(fp, "filter            = none\n");
                std::fprintf(fp, "contrast_day      = 1.000000\n");
                std::fprintf(fp, "contrast_night    = 1.000000\n");
                std::fprintf(fp, "gamma_day         = 2.400000\n");
                std::fprintf(fp, "gamma_night       = 2.400000\n");
                std::fprintf(fp, "luminance_day     = 0.000000\n");
                std::fprintf(fp, "luminance_night   = 0.000000\n");
                std::fprintf(fp, "range_day         = 0.00-1.00\n");
                std::fprintf(fp, "range_night       = 0.00-1.00\n");
                std::fprintf(fp, "dimming_timeout   = 00:00\n");
            }
            std::fclose(fp);
        }

        FizeauProfile def = {};
        def.day_settings   = Config::default_settings;
        def.night_settings = Config::default_settings;
        def.dusk_begin     = { 20, 0, 0 };
        def.dusk_end       = { 20, 0, 0 };
        def.dawn_begin     = {  7, 0, 0 };
        def.dawn_end       = {  7, 0, 0 };
        def.components     = Component_All;
        def.filter         = Component_None;
        def.dimming_timeout = {};

        for (std::size_t i = existing; i < FizeauProfileId_Total; ++i)
            fizeauSetProfile(static_cast<FizeauProfileId>(i), &def);
    }

    static std::size_t count_profiles_in_config() {
        FILE *fp = std::fopen(config_ini_path, "r");
        if (!fp) return 1;

        std::array<bool, FizeauProfileId_Total> seen{};
        char line[128];
        while (std::fgets(line, sizeof(line), fp)) {
            int id;
            if (std::sscanf(line, " [profile%d]", &id) == 1 &&
                id >= 1 && id <= FizeauProfileId_Total) {
                seen[id - 1] = true;
            }
        }
        std::fclose(fp);

        std::size_t count = 0;
        for (bool s : seen) {
            if (!s) break;
            ++count;
        }
        return std::max<std::size_t>(count, 1);
    }

    void save_period_overrides() {
        FILE *fp = std::fopen(overrides_path, "w");
        if (!fp) return;
        for (std::size_t i = 0; i < this->num_profiles; ++i) {
            auto &ps = this->period_states[i];
            if (ps.override == PeriodOverride::Dynamic) {
                std::fprintf(fp, "profile%zu=dynamic\n", i + 1);
            } else {
                const char *ov_str = (ps.override == PeriodOverride::Day) ? "day" : "night";
                std::fprintf(fp, "profile%zu=%s,%02d:%02d,%02d:%02d,%02d:%02d,%02d:%02d\n",
                    i + 1, ov_str,
                    (int)ps.real_dusk_begin.h, (int)ps.real_dusk_begin.m,
                    (int)ps.real_dusk_end.h,   (int)ps.real_dusk_end.m,
                    (int)ps.real_dawn_begin.h, (int)ps.real_dawn_begin.m,
                    (int)ps.real_dawn_end.h,   (int)ps.real_dawn_end.m);
            }
        }
        std::fclose(fp);
    }

    void load_period_overrides() {
        FILE *fp = std::fopen(overrides_path, "r");
        if (!fp) return;

        char line[128];
        while (std::fgets(line, sizeof(line), fp)) {
            int  id;
            char ov_str[16];
            char db[8] = {}, de[8] = {}, ab[8] = {}, ae[8] = {};

            int n = std::sscanf(line, "profile%d=%15[^,],%7[^,],%7[^,],%7[^,],%7s",
                                &id, ov_str, db, de, ab, ae);
            if (n < 2 || id < 1 || id > (int)this->num_profiles)
                continue;

            auto idx = id - 1;
            auto &ps = this->period_states[idx];

            if      (std::strcmp(ov_str, "day")   == 0) ps.override = PeriodOverride::Day;
            else if (std::strcmp(ov_str, "night") == 0) ps.override = PeriodOverride::Night;
            else                                         ps.override = PeriodOverride::Dynamic;

            if (n == 6 && ps.override != PeriodOverride::Dynamic) {
                auto parse_t = [](const char *s) -> Time {
                    Time t = {}; int h = 0, m = 0;
                    std::sscanf(s, "%d:%d", &h, &m);
                    t.h = h; t.m = m;
                    return t;
                };
                ps.real_dusk_begin = parse_t(db);
                ps.real_dusk_end   = parse_t(de);
                ps.real_dawn_begin = parse_t(ab);
                ps.real_dawn_end   = parse_t(ae);
            }
        }
        std::fclose(fp);
    }

    void set_period_override(FizeauProfileId id, PeriodOverride new_ov) {
        if (id >= FizeauProfileId_Total) return;

        auto &ps = this->period_states[id];

        if (ps.override == PeriodOverride::Dynamic && new_ov != PeriodOverride::Dynamic) {
            ps.real_dusk_begin = this->config.profile.dusk_begin;
            ps.real_dusk_end   = this->config.profile.dusk_end;
            ps.real_dawn_begin = this->config.profile.dawn_begin;
            ps.real_dawn_end   = this->config.profile.dawn_end;
        }

        ps.override = new_ov;

        switch (new_ov) {
            case PeriodOverride::Day:
                this->config.profile.dusk_begin = OVERRIDE_DAY_DUSK_BEGIN;
                this->config.profile.dusk_end   = OVERRIDE_DAY_DUSK_END;
                this->config.profile.dawn_begin = OVERRIDE_DAY_DAWN_BEGIN;
                this->config.profile.dawn_end   = OVERRIDE_DAY_DAWN_END;
                break;
            case PeriodOverride::Night:
                this->config.profile.dusk_begin = OVERRIDE_NIGHT_DUSK_BEGIN;
                this->config.profile.dusk_end   = OVERRIDE_NIGHT_DUSK_END;
                this->config.profile.dawn_begin = OVERRIDE_NIGHT_DAWN_BEGIN;
                this->config.profile.dawn_end   = OVERRIDE_NIGHT_DAWN_END;
                break;
            case PeriodOverride::Dynamic:
                this->config.profile.dusk_begin = ps.real_dusk_begin;
                this->config.profile.dusk_end   = ps.real_dusk_end;
                this->config.profile.dawn_begin = ps.real_dawn_begin;
                this->config.profile.dawn_end   = ps.real_dawn_end;
                break;
        }

        this->config.apply();
    }

    Result apply_with_override() {
        return this->config.apply();
    }

    void switch_profile(FizeauProfileId new_id) {
        if (this->pending_apply) {
            this->config.apply();
            this->pending_apply = false;
            this->apply_counter = 0;
        }

        if (this->rc = this->config.open_profile(new_id); R_FAILED(this->rc))
            return;

        auto &ps = this->period_states[new_id];
        if (ps.override == PeriodOverride::Dynamic) {
            ps.real_dusk_begin = this->config.profile.dusk_begin;
            ps.real_dusk_end   = this->config.profile.dusk_end;
            ps.real_dawn_begin = this->config.profile.dawn_begin;
            ps.real_dawn_end   = this->config.profile.dawn_end;
        }

        bool is_external = (this->perf_mode != ApmPerformanceMode_Normal);
        fizeauSetActiveProfileId(is_external, new_id);
        (is_external ? this->config.external_profile : this->config.internal_profile) = new_id;

        this->refresh_sliders();
    }

    virtual tsl::elm::Element *createUI() override {
        this->info_header = new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
            renderer->drawString(format(str::PERIOD_FMT(), this->is_day ? str::PERIOD_DAY() : str::PERIOD_NIGHT()).c_str(),
                false, x, y + 20, 20, (0xffff));
        });

        // Profile bar (multi-profile only)
        if (this->num_profiles > 1) {
            std::vector<std::string> labels;
            labels.reserve(this->num_profiles);
            for (std::size_t i = 1; i <= this->num_profiles; ++i)
                labels.push_back(std::to_string(i));
            this->profile_bar = new DynamicProfileTrackBar(std::move(labels), str::DISPLAY_PROFILE());
            this->profile_bar->setProgress(static_cast<u8>(this->config.cur_profile_id));
            this->profile_bar->setValueChangedListener([this](u8 val) {
                auto new_id = static_cast<FizeauProfileId>(val);
                if (new_id != this->config.cur_profile_id)
                    this->switch_profile(new_id);
            });
        }

        // Correction toggle
        this->active_button = new tsl::elm::ListItem(str::CORRECTION_STATE());
        this->active_button->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_A) {
                this->config.active ^= 1;
                this->rc = fizeauSetIsActive(this->config.active);
                this->active_button->setValue(this->config.active ? str::ACTIVE() : str::INACTIVE());
                return true;
            }
            return false;
        });
        this->active_button->setValue(this->config.active ? str::ACTIVE() : str::INACTIVE());

        // Period mode cycle: Авто → День → Ночь → Авто
        this->period_button = new tsl::elm::ListItem(str::PERIOD_MODE());
        this->period_button->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_A) {
                auto id = this->config.cur_profile_id;
                if (id >= FizeauProfileId_Total)
                    return false;

                auto new_ov = static_cast<PeriodOverride>(
                    (static_cast<std::uint8_t>(this->period_states[id].override) + 1) % 3);

                this->set_period_override(id, new_ov);
                this->period_button->setValue(
                    (new_ov == PeriodOverride::Day)   ? str::MODE_DAY()   :
                    (new_ov == PeriodOverride::Night) ? str::MODE_NIGHT() :
                                                        str::MODE_DYNAMIC());
                this->refresh_sliders();
                this->save_period_overrides();
                return true;
            }
            return false;
        });
        {
            auto id = this->config.cur_profile_id;
            auto ov = (id < FizeauProfileId_Total) ? this->period_states[id].override : PeriodOverride::Dynamic;
            this->period_button->setValue(
                (ov == PeriodOverride::Day)   ? str::MODE_DAY()   :
                (ov == PeriodOverride::Night) ? str::MODE_NIGHT() :
                                                str::MODE_DYNAMIC());
        }

        // Dawn slider
        this->dawn_slider = new TimeStepTrackBar(str::DAY_START());
        {
            auto id = this->config.cur_profile_id;
            auto &ps = this->period_states[id < FizeauProfileId_Total ? id : 0];
            this->dawn_slider->setProgress(TimeStepTrackBar::time_to_hour(ps.real_dawn_begin));
        }
        this->dawn_slider->setValueChangedListener([this](u16 hour) {
            auto id = this->config.cur_profile_id;
            if (id >= FizeauProfileId_Total) return;
            auto &ps = this->period_states[id];
            if (ps.override != PeriodOverride::Dynamic) return;
            Time t = TimeStepTrackBar::hour_to_time(hour);
            ps.real_dawn_begin = ps.real_dawn_end = t;
            this->config.profile.dawn_begin = this->config.profile.dawn_end = t;
            this->pending_apply = true;
        });

        // Dusk slider
        this->dusk_slider = new TimeStepTrackBar(str::DAY_END());
        {
            auto id = this->config.cur_profile_id;
            auto &ps = this->period_states[id < FizeauProfileId_Total ? id : 0];
            this->dusk_slider->setProgress(TimeStepTrackBar::time_to_hour(ps.real_dusk_begin));
        }
        this->dusk_slider->setValueChangedListener([this](u16 hour) {
            auto id = this->config.cur_profile_id;
            if (id >= FizeauProfileId_Total) return;
            auto &ps = this->period_states[id];
            if (ps.override != PeriodOverride::Dynamic) return;
            Time t = TimeStepTrackBar::hour_to_time(hour);
            ps.real_dusk_begin = ps.real_dusk_end = t;
            this->config.profile.dusk_begin = this->config.profile.dusk_end = t;
            this->pending_apply = true;
        });

        // Reset button
        this->reset_button = new tsl::elm::ListItem(str::RESET_SETTINGS());
        this->reset_button->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_A) {
                auto reset_f = [](FizeauSettings &s) {
                    s.temperature = DEFAULT_TEMP;
                    s.saturation  = DEFAULT_SAT;
                    s.hue         = DEFAULT_HUE;
                    s.contrast    = DEFAULT_CONTRAST;
                    s.gamma       = DEFAULT_GAMMA;
                    s.luminance   = DEFAULT_LUMA;
                    s.range       = DEFAULT_RANGE;
                };
                reset_f(this->config.profile.day_settings);
                reset_f(this->config.profile.night_settings);
                this->config.profile.components = Component_All;
                this->config.profile.filter     = Component_None;
                this->rc = this->apply_with_override();
                this->pending_apply = false;
                this->apply_counter = 0;
                this->refresh_sliders();
                return true;
            }
            return false;
        });

        // Temperature
        this->temp_slider = new tsl::elm::TrackBar("");
        this->temp_slider->setProgress(((this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature) - MIN_TEMP)
            * 100 / ((this->allow_high_temp ? MAX_TEMP : D65_TEMP) - MIN_TEMP));
        this->temp_slider->setClickListener([&, this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->temp_slider->setProgress((DEFAULT_TEMP - MIN_TEMP) * 100 / ((this->allow_high_temp ? MAX_TEMP : D65_TEMP) - MIN_TEMP));
                (this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature) = DEFAULT_TEMP;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->temp_slider->setValueChangedListener([this](std::uint8_t val) {
            (this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature) =
                val * ((this->allow_high_temp ? MAX_TEMP : D65_TEMP) - MIN_TEMP) / 100 + MIN_TEMP;
            this->pending_apply = true;
        });

        // Saturation
        this->sat_slider = new tsl::elm::TrackBar("");
        this->sat_slider->setProgress(((this->is_day ? this->config.profile.day_settings.saturation : this->config.profile.night_settings.saturation) - MIN_SAT)
            * 100 / (MAX_SAT - MIN_SAT));
        this->sat_slider->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->sat_slider->setProgress((DEFAULT_SAT - MIN_SAT) * 100 / (MAX_SAT - MIN_SAT));
                (this->is_day ? this->config.profile.day_settings.saturation : this->config.profile.night_settings.saturation) = DEFAULT_SAT;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->sat_slider->setValueChangedListener([this](std::uint8_t val) {
            (this->is_day ? this->config.profile.day_settings.saturation : this->config.profile.night_settings.saturation) =
                val * (MAX_SAT - MIN_SAT) / 100 + MIN_SAT;
            this->pending_apply = true;
        });

        // Hue
        this->hue_slider = new tsl::elm::TrackBar("");
        this->hue_slider->setProgress(((this->is_day ? this->config.profile.day_settings.hue : this->config.profile.night_settings.hue) - MIN_HUE)
            * 100 / (MAX_HUE - MIN_HUE));
        this->hue_slider->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->hue_slider->setProgress((DEFAULT_HUE - MIN_HUE) * 100 / (MAX_HUE - MIN_HUE));
                (this->is_day ? this->config.profile.day_settings.hue : this->config.profile.night_settings.hue) = DEFAULT_HUE;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->hue_slider->setValueChangedListener([this](std::uint8_t val) {
            (this->is_day ? this->config.profile.day_settings.hue : this->config.profile.night_settings.hue) =
                val * (MAX_HUE - MIN_HUE) / 100 + MIN_HUE;
            this->pending_apply = true;
        });

        // Components
        this->components_bar = new tsl::elm::NamedStepTrackBar("", {
            str::COMP_NONE(), "R", "G", "RG", "B", "RB", "GB", str::COMP_ALL() });
        this->components_bar->setProgress(static_cast<u8>(this->config.profile.components));
        this->components_bar->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->components_bar->setProgress(Component_All);
                this->config.profile.components = Component_All;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->components_bar->setValueChangedListener([this](u8 val) {
            this->config.profile.components = static_cast<Component>(val);
            this->pending_apply = true;
        });

        // Filter
        this->filter_bar = new tsl::elm::NamedStepTrackBar("", {
            str::FILTER_NONE(), str::FILTER_RED(), str::FILTER_GREEN(), str::FILTER_BLUE() });
        this->filter_bar->setProgress((this->config.profile.filter == Component_None) ? 0 : std::countr_zero(static_cast<std::uint32_t>(this->config.profile.filter)) + 1);
        this->filter_bar->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->filter_bar->setProgress(Component_None);
                this->config.profile.filter = Component_None;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->filter_bar->setValueChangedListener([this](u8 val) {
            this->config.profile.filter = static_cast<Component>(static_cast<Component>(val ? BIT(val - 1) : val));
            this->pending_apply = true;
        });

        // Contrast
        this->contrast_slider = new tsl::elm::TrackBar("");
        this->contrast_slider->setProgress(((this->is_day ? this->config.profile.day_settings.contrast : this->config.profile.night_settings.contrast) - MIN_CONTRAST)
            * 100 / (MAX_CONTRAST - MIN_CONTRAST));
        this->contrast_slider->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->contrast_slider->setProgress((DEFAULT_CONTRAST - MIN_CONTRAST) * 100 / (MAX_CONTRAST - MIN_CONTRAST));
                (this->is_day ? this->config.profile.day_settings.contrast : this->config.profile.night_settings.contrast) = DEFAULT_CONTRAST;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->contrast_slider->setValueChangedListener([this](std::uint8_t val) {
            (this->is_day ? this->config.profile.day_settings.contrast : this->config.profile.night_settings.contrast) =
                val * (MAX_CONTRAST - MIN_CONTRAST) / 100 + MIN_CONTRAST;
            this->pending_apply = true;
        });

        // Gamma
        this->gamma_slider = new tsl::elm::TrackBar("");
        this->gamma_slider->setProgress(((this->is_day ? this->config.profile.day_settings.gamma : this->config.profile.night_settings.gamma) - MIN_GAMMA)
            * 100 / (MAX_GAMMA - MIN_GAMMA));
        this->gamma_slider->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->gamma_slider->setProgress((DEFAULT_GAMMA - MIN_GAMMA) * 100 / (MAX_GAMMA - MIN_GAMMA));
                (this->is_day ? this->config.profile.day_settings.gamma : this->config.profile.night_settings.gamma) = DEFAULT_GAMMA;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->gamma_slider->setValueChangedListener([this](std::uint8_t val) {
            (this->is_day ? this->config.profile.day_settings.gamma : this->config.profile.night_settings.gamma) =
                val * (MAX_GAMMA - MIN_GAMMA) / 100 + MIN_GAMMA;
            this->pending_apply = true;
        });

        // Luminance
        this->luma_slider = new tsl::elm::TrackBar("");
        this->luma_slider->setProgress(((this->is_day ? this->config.profile.day_settings.luminance : this->config.profile.night_settings.luminance) - MIN_LUMA)
            * 100 / (MAX_LUMA - MIN_LUMA));
        this->luma_slider->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_Y) {
                this->luma_slider->setProgress((DEFAULT_LUMA - MIN_LUMA) * 100 / (MAX_LUMA - MIN_LUMA));
                (this->is_day ? this->config.profile.day_settings.luminance : this->config.profile.night_settings.luminance) = DEFAULT_LUMA;
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->luma_slider->setValueChangedListener([this](std::uint8_t val) {
            (this->is_day ? this->config.profile.day_settings.luminance : this->config.profile.night_settings.luminance) =
                val * (MAX_LUMA - MIN_LUMA) / 100 + MIN_LUMA;
            this->pending_apply = true;
        });

        // Color range
        this->range_button = new tsl::elm::ListItem(str::COLOR_RANGE());
        this->range_button->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_A) {
                auto &range = (this->is_day ? this->config.profile.day_settings.range : this->config.profile.night_settings.range);
                if (is_full(range))
                    range = DEFAULT_LIMITED_RANGE;
                else
                    range = DEFAULT_RANGE;
                this->range_button->setValue(is_full(range) ? str::FULL() : str::LIMITED());
                this->rc = this->apply_with_override();
                this->pending_apply = false; this->apply_counter = 0;
                return true;
            }
            return false;
        });
        this->range_button->setValue(is_full(this->is_day ? this->config.profile.day_settings.range : this->config.profile.night_settings.range) ? str::FULL() : str::LIMITED());

        // Language toggle (RU ↔ EN) — rebuilds full UI so all strings update
        auto* lang_button = new tsl::elm::ListItem(str::LANGUAGE());
        lang_button->setClickListener([](std::uint64_t keys) {
            if (keys & HidNpadButton_A) {
                g_lang = (g_lang == Lang::RU) ? Lang::EN : Lang::RU;
                tsl::changeTo<FizeauOverlayGui>();
                return true;
            }
            return false;
        });

        // Presets button
        auto* presets_button = new tsl::elm::ListItem(str::PRESETS_MENU());
        presets_button->setClickListener([this](std::uint64_t keys) {
            if (keys & HidNpadButton_A) {
                tsl::changeTo<PresetsGui>(
                    &this->config.profile.day_settings,
                    &this->config.profile.night_settings,
                    &this->is_day,
                    &this->config);
                return true;
            }
            return false;
        });

        // Category headers
        this->temp_header       = new tsl::elm::CategoryHeader(str::TEMPERATURE());
        this->sat_header        = new tsl::elm::CategoryHeader(str::SATURATION());
        this->hue_header        = new tsl::elm::CategoryHeader(str::HUE());
        this->components_header = new tsl::elm::CategoryHeader(str::COMPONENTS());
        this->filter_header     = new tsl::elm::CategoryHeader(str::FILTER());
        this->contrast_header   = new tsl::elm::CategoryHeader(str::CONTRAST());
        this->gamma_header      = new tsl::elm::CategoryHeader(str::GAMMA());
        this->luma_header       = new tsl::elm::CategoryHeader(str::LUMINANCE());

        auto* list = new tsl::elm::List();

        this->display_settings_header = new tsl::elm::CategoryHeader(str::DISPLAY_SETTINGS());
        list->addItem(this->display_settings_header);
        list->addItem(this->active_button);
        list->addItem(this->reset_button);
        list->addItem(presets_button);
        list->addItem(lang_button);
        if (this->profile_bar)
            list->addItem(this->profile_bar);
        this->daylight_header = new tsl::elm::CategoryHeader(str::DAYLIGHT_CYCLE());
        list->addItem(this->daylight_header);
        list->addItem(this->period_button);
        list->addItem(this->dawn_slider);
        list->addItem(this->dusk_slider);
        list->addItem(this->temp_header);
        list->addItem(this->temp_slider);
        list->addItem(this->sat_header);
        list->addItem(this->sat_slider);
        list->addItem(this->hue_header);
        list->addItem(this->hue_slider);
        list->addItem(this->components_header);
        list->addItem(this->components_bar);
        list->addItem(this->filter_header);
        list->addItem(this->filter_bar);
        list->addItem(this->contrast_header);
        list->addItem(this->contrast_slider);
        list->addItem(this->gamma_header);
        list->addItem(this->gamma_slider);
        list->addItem(this->luma_header);
        list->addItem(this->luma_slider);
        list->addItem(this->range_button);

        auto* frame = new tsl::elm::OverlayFrame("Fizeau", VERSION);
        frame->setContent(list);

        #if USING_WIDGET_DIRECTIVE
        frame->m_showWidget = true;
        #endif

        return frame;
    }

    virtual void update() override {
        if (R_FAILED(this->rc) && this->config.cur_profile_id == FizeauProfileId_Invalid)
            tsl::changeTo<ErrorGui>(this->rc);

        this->is_day = this->compute_is_day();

        this->display_mode_poll_counter++;
        if (this->display_mode_poll_counter >= 18) {
            this->display_mode_poll_counter = 0;
            apmGetPerformanceMode(&this->perf_mode);
        }
        this->display_settings_header->setValue(
            this->perf_mode == ApmPerformanceMode_Normal ? str::HANDHELD() : str::DOCKED(),
            tsl::onTextColor);

        if (this->pending_apply) {
            this->apply_counter++;
            if (this->apply_counter >= 3) {
                Result apply_rc = this->apply_with_override();
                if (R_FAILED(apply_rc)) {
                    LOG("Failed to apply config: %#x\n", apply_rc);
                }
                this->pending_apply = false;
                this->apply_counter = 0;
            }
        }

        this->temp_header->setValue(format("%u°K",
            this->is_day ? this->config.profile.day_settings.temperature : this->config.profile.night_settings.temperature), tsl::onTextColor);
        this->daylight_header->setValue(this->is_day ? str::MODE_DAY() : str::MODE_NIGHT(), tsl::onTextColor);
        this->sat_header->setValue(format("%.2f",
            this->is_day ? this->config.profile.day_settings.saturation  : this->config.profile.night_settings.saturation), tsl::onTextColor);
        this->hue_header->setValue(format("%.2f",
            this->is_day ? this->config.profile.day_settings.hue         : this->config.profile.night_settings.hue), tsl::onTextColor);
        this->contrast_header->setValue(format("%.2f",
            this->is_day ? this->config.profile.day_settings.contrast    : this->config.profile.night_settings.contrast), tsl::onTextColor);
        this->gamma_header->setValue(format("%.2f",
            this->is_day ? this->config.profile.day_settings.gamma       : this->config.profile.night_settings.gamma), tsl::onTextColor);
        this->luma_header->setValue(format("%.2f",
            this->is_day ? this->config.profile.day_settings.luminance   : this->config.profile.night_settings.luminance), tsl::onTextColor);
    }

    Config &get_config() { return this->config; }

private:
    Result rc;
    bool   is_day;
    bool   allow_high_temp;
    ApmPerformanceMode perf_mode = ApmPerformanceMode_Normal;
    Config config = {};

    tsl::elm::CustomDrawer      *info_header             = nullptr;
    DynamicProfileTrackBar      *profile_bar             = nullptr;
    tsl::elm::ListItem          *active_button           = nullptr;
    tsl::elm::ListItem          *period_button           = nullptr;
    TimeStepTrackBar            *dawn_slider             = nullptr;
    TimeStepTrackBar            *dusk_slider             = nullptr;
    tsl::elm::ListItem          *reset_button            = nullptr;
    tsl::elm::TrackBar          *temp_slider             = nullptr;
    tsl::elm::TrackBar          *sat_slider              = nullptr;
    tsl::elm::TrackBar          *hue_slider              = nullptr;
    tsl::elm::NamedStepTrackBar *components_bar          = nullptr;
    tsl::elm::NamedStepTrackBar *filter_bar              = nullptr;
    tsl::elm::TrackBar          *contrast_slider         = nullptr;
    tsl::elm::TrackBar          *gamma_slider            = nullptr;
    tsl::elm::TrackBar          *luma_slider             = nullptr;
    tsl::elm::ListItem          *range_button            = nullptr;

    tsl::elm::CategoryHeader *temp_header, *sat_header, *hue_header,
        *components_header, *filter_header, *contrast_header, *gamma_header, *luma_header;
    tsl::elm::CategoryHeader *daylight_header         = nullptr;
    tsl::elm::CategoryHeader *display_settings_header = nullptr;

    int  display_mode_poll_counter = 0;
    int  apply_counter;
    bool pending_apply;

    std::array<ProfilePeriodState, FizeauProfileId_Total> period_states = {};
    std::size_t num_profiles = 1;
};

} // namespace fz


// ========================================
// Main Overlay Class
// ========================================
class FizeauOverlay: public tsl::Overlay {
private:
    bool serviceActive = true;

public:
    virtual void initServices() override {
#ifdef DEBUG
        twiliInitialize();
        twiliCreateNamedOutputPipe(&g_twlPipe, "fzovlout");
#endif
        apmInitialize();

        bool is_active = false;
        Result rc = fizeauIsServiceActive(&is_active);

        if (R_FAILED(rc) || !is_active) {
            serviceActive = false;
            return;
        }

        fz::Clock::initialize();
    }

    virtual void exitServices() override {
        apmExit();
#ifdef DEBUG
        twiliClosePipe(&g_twlPipe);
        twiliExit();
#endif
    }

    virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
        if (!serviceActive)
            return initially<fz::ServiceInactiveGui>();

        return initially<fz::FizeauOverlayGui>();
    }

    virtual void onShow() override { }
    virtual void onHide() override { }
};


// ========================================
// Main Entry Point
// ========================================
int main(int argc, char **argv) {
    LOG("Starting overlay\n");
    return tsl::loop<FizeauOverlay>(argc, argv);
}
