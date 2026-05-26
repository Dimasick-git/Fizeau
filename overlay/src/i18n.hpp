// Fizeau Russian/English localization
// RU by default; toggle via language button in overlay

#pragma once
#include <string_view>

enum class Lang : std::uint8_t { RU = 0, EN = 1 };
inline Lang g_lang = Lang::RU;

// T(russian, english) — returns the string for the active language
#define T(ru, en) (g_lang == Lang::RU ? (ru) : (en))

namespace str {
    // ── Header info ──────────────────────────────────────────────────────────
    inline const char* PERIOD_DAY()   { return T("день",  "day"); }
    inline const char* PERIOD_NIGHT() { return T("ночь",  "night"); }
    inline const char* PERIOD_FMT()   { return T("Период: %s",   "Period: %s"); }

    // ── Buttons ───────────────────────────────────────────────────────────────
    inline const char* CORRECTION_STATE() { return T("Коррекция",          "Correction State"); }
    inline const char* PERIOD_MODE()      { return T("Режим периода",       "Period Mode"); }
    inline const char* RESET_SETTINGS()   { return T("Сбросить настройки", "Reset Settings"); }
    inline const char* COLOR_RANGE()      { return T("Диапазон цвета",     "Color Range"); }
    inline const char* LANGUAGE()         { return T("Язык: RU ► EN", "Language: EN ► RU"); }
    inline const char* PRESETS_MENU()     { return T("Пресеты",            "Presets"); }

    // ── Button values ─────────────────────────────────────────────────────────
    inline const char* ACTIVE()    { return T("Активна",       "Active"); }
    inline const char* INACTIVE()  { return T("Неактивна",     "Inactive"); }
    inline const char* FULL()      { return T("Полный",        "Full"); }
    inline const char* LIMITED()   { return T("Ограниченный",  "Limited"); }
    inline const char* MODE_DAY()     { return T("День",    "Day"); }
    inline const char* MODE_NIGHT()   { return T("Ночь",    "Night"); }
    inline const char* MODE_DYNAMIC() { return T("Авто",    "Dynamic"); }
    inline const char* HANDHELD()  { return T("Ручной",   "Handheld"); }
    inline const char* DOCKED()    { return T("Докстанция","Docked"); }

    // ── Category headers ──────────────────────────────────────────────────────
    inline const char* DISPLAY_SETTINGS() { return T("Настройки экрана",   "Display Settings"); }
    inline const char* DAYLIGHT_CYCLE()   { return T("Цикл освещения",     "Daylight Cycle"); }
    inline const char* DAY_START()        { return T("Начало дня",         "Day Start / Night End"); }
    inline const char* DAY_END()          { return T("Конец дня",          "Day End / Night Start"); }
    inline const char* DISPLAY_PROFILE()  { return T("Профиль экрана",     "Display Profile"); }
    inline const char* TEMPERATURE()      { return T("Температура",        "Temperature"); }
    inline const char* SATURATION()       { return T("Насыщенность",       "Saturation"); }
    inline const char* HUE()              { return T("Оттенок",            "Hue"); }
    inline const char* COMPONENTS()       { return T("Компоненты",         "Components"); }
    inline const char* FILTER()           { return T("Фильтр",             "Filter"); }
    inline const char* CONTRAST()         { return T("Контраст",           "Contrast"); }
    inline const char* GAMMA()            { return T("Гамма",              "Gamma"); }
    inline const char* LUMINANCE()        { return T("Яркость",            "Luminance"); }

    // ── Error / service screens ───────────────────────────────────────────────
    inline const char* SERVICE_NOT_ACTIVE_1() { return T("Системный модуль Fizeau неактивен.",          "Fizeau system module is not active."); }
    inline const char* SERVICE_NOT_ACTIVE_2() { return T("Включите системный модуль и",                  "Enable the system module and"); }
    inline const char* SERVICE_NOT_ACTIVE_3() { return T("перезагрузите консоль.",                        "reboot your device."); }
    inline const char* ERROR_OCCURRED()       { return T("Произошла ошибка",                              "An error occurred"); }
    inline const char* ERROR_USE_LATEST_1()   { return T("Убедитесь, что вы используете",                 "Please make sure you are using the"); }
    inline const char* ERROR_USE_LATEST_2()   { return T("последнюю версию.",                             "latest release."); }
    inline const char* ERROR_REPORT()         { return T("Сообщите о проблеме на github:",                "Otherwise, make an issue on github:"); }

    // ── Presets screen ────────────────────────────────────────────────────────
    inline const char* PRESETS_TITLE()     { return T("Пресеты цвета",    "Color Presets"); }
    inline const char* SAVE_CURRENT()      { return T("Сохранить текущие","Save Current"); }
    inline const char* APPLY_PRESET()      { return T("A: применить",     "A: apply"); }
    inline const char* APPLY_DEL_PRESET()  { return T("A: применить  X: удалить", "A: apply  X: delete"); }
    inline const char* BUILTIN_LABEL()     { return T("Встроенные",       "Built-in"); }
    inline const char* CUSTOM_LABEL()      { return T("Пользовательские", "Custom"); }

    // ── Components / Filter labels ────────────────────────────────────────────
    inline const char* COMP_NONE()        { return T("Нет",           "None"); }
    inline const char* COMP_ALL()         { return T("Все",           "All"); }
    inline const char* FILTER_NONE()      { return T("Нет",           "None"); }
    inline const char* FILTER_RED()       { return T("Красный",       "Red"); }
    inline const char* FILTER_GREEN()     { return T("Зелёный",       "Green"); }
    inline const char* FILTER_BLUE()      { return T("Синий",         "Blue"); }
} // namespace str
