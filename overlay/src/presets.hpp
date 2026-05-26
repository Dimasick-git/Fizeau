// Fizeau preset system — built-in and user-defined color presets

#pragma once
#include <string>
#include <vector>
#include <cstdio>
#include <types.h>

struct FzPreset {
    std::string name;
    bool        is_builtin = false;
    Temperature temperature;
    float       saturation;
    float       hue;
    float       contrast;
    float       gamma;
    float       luminance;
};

// 5 built-in presets (day settings; same values applied to night on request)
inline const std::vector<FzPreset> BUILTIN_PRESETS = {
    { "Стандарт",   true, 6500, 1.00f, 0.00f, 1.00f, 2.40f,  0.00f },
    { "Яркий",      true, 6500, 1.30f, 0.00f, 1.20f, 2.20f,  0.20f },
    { "Насыщенный", true, 6500, 1.60f, 0.00f, 1.00f, 2.40f,  0.00f },
    { "Ночной",     true, 3200, 0.90f, 0.00f, 0.90f, 2.60f, -0.10f },
    { "Мягкий",     true, 5500, 0.80f, 0.00f, 0.90f, 2.50f, -0.10f },
};

class PresetManager {
public:
    static constexpr const char *PATH = "/config/fizeau/presets.ini";

    void load() {
        this->custom.clear();
        FILE *fp = std::fopen(PATH, "r");
        if (!fp) return;
        char line[256];
        FzPreset p{};
        while (std::fgets(line, sizeof(line), fp)) {
            if (line[0] == '[') {
                if (!p.name.empty())
                    this->custom.push_back(p);
                p = {};
                char name[64] = {};
                std::sscanf(line, "[%63[^]]]", name);
                p.name = name;
            } else {
                float fv = 0.f;
                int   iv = 0;
                if (std::sscanf(line, "temperature=%d", &iv) == 1)       p.temperature  = (Temperature)iv;
                else if (std::sscanf(line, "saturation=%f",  &fv) == 1)  p.saturation  = fv;
                else if (std::sscanf(line, "hue=%f",         &fv) == 1)  p.hue         = fv;
                else if (std::sscanf(line, "contrast=%f",    &fv) == 1)  p.contrast    = fv;
                else if (std::sscanf(line, "gamma=%f",       &fv) == 1)  p.gamma       = fv;
                else if (std::sscanf(line, "luminance=%f",   &fv) == 1)  p.luminance   = fv;
            }
        }
        if (!p.name.empty())
            this->custom.push_back(p);
        std::fclose(fp);
    }

    void save() const {
        FILE *fp = std::fopen(PATH, "w");
        if (!fp) return;
        for (auto &p : this->custom) {
            std::fprintf(fp, "[%s]\n",          p.name.c_str());
            std::fprintf(fp, "temperature=%u\n", (unsigned)p.temperature);
            std::fprintf(fp, "saturation=%.6f\n", p.saturation);
            std::fprintf(fp, "hue=%.6f\n",         p.hue);
            std::fprintf(fp, "contrast=%.6f\n",    p.contrast);
            std::fprintf(fp, "gamma=%.6f\n",        p.gamma);
            std::fprintf(fp, "luminance=%.6f\n",    p.luminance);
        }
        std::fclose(fp);
    }

    // Save current day/night settings as a new custom preset
    void saveCurrentAs(Temperature temp, float sat, float hue, float contrast, float gamma, float luminance) {
        FzPreset p;
        p.name        = "Пресет " + std::to_string(this->custom.size() + 1);
        p.is_builtin  = false;
        p.temperature = temp;
        p.saturation  = sat;
        p.hue         = hue;
        p.contrast    = contrast;
        p.gamma       = gamma;
        p.luminance   = luminance;
        this->custom.push_back(p);
        this->save();
    }

    void removeCustom(std::size_t idx) {
        if (idx < this->custom.size()) {
            this->custom.erase(this->custom.begin() + idx);
            this->save();
        }
    }

    // All presets: built-in first, then custom
    std::vector<FzPreset> all() const {
        std::vector<FzPreset> result(BUILTIN_PRESETS.begin(), BUILTIN_PRESETS.end());
        result.insert(result.end(), this->custom.begin(), this->custom.end());
        return result;
    }

    std::vector<FzPreset> custom;
};

inline PresetManager g_presets;
