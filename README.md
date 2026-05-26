# Fizeau — Ряженка rework

> Форк [ppkantorski/Fizeau](https://github.com/ppkantorski/Fizeau) с полной русификацией, системой пресетов и интеграцией [libryazhahand](https://github.com/Dimasick-git/libryazhahand).

---

## Что это

**Fizeau** — Tesla-оверлей для Nintendo Switch (Atmosphere CFW), управляющий цветовым блоком CMU (Color Management Unit) в GPU Tegra. Позволяет прямо из любой игры мгновенно настроить:

- температуру цвета (тёплый/холодный экран)
- насыщенность, оттенок, контраст, гамму, яркость
- цветовые компоненты и фильтры
- цикл день/ночь по расписанию или вручную

---

## Возможности v2.9.2

### Основные
- Управление цветом экрана через CMU Tegra в реальном времени
- До 4 независимых профилей (портативный / докстанция)
- Раздельные настройки для дня и ночи в каждом профиле
- Режим периода: **Авто** / **День** / **Ночь** (переключение одной кнопкой)
- Слайдеры времени начала и конца дня (по часам)
- Сброс всех параметров одной кнопкой

### Пресеты
- **5 встроенных пресетов**: Стандарт, Яркий, Насыщенный, Ночной, Мягкий
- Сохранение текущих настроек как пользовательского пресета
- Удаление пользовательских пресетов (кнопка Y)
- Отдельный экран пресетов

### Русификация
- Полный перевод всего интерфейса на русский язык
- Кнопка переключения **RU ↔ EN** прямо в меню

### Сборка и релизы
- Автосборка через GitHub Actions при каждом push
- Авторелиз в GitHub Releases при создании тега `v*`
- Собирается на **libryazhahand** (форк libultrahand от команды Ryazhenka)

---

## Установка

1. Скачай `Fizeau-2.9.2.zip` из [Releases](https://github.com/Dimasick-git/Fizeau/releases)
2. Распакуй архив в **корень SD-карты** (структура папок сохранится автоматически)
3. Перезагрузи консоль
4. Открой Fizeau через Ryazhahand (или любой другой Tesla-лаунчер)

Конфиг хранится в `/config/fizeau/config.ini`

---

## Параметры настройки

| Параметр | Диапазон | По умолчанию |
|---|---|---|
| Температура | 1000 — 10000 K | 6500 K |
| Насыщенность | 0.0 — 2.0 | 1.0 |
| Оттенок | -1.0 — 1.0 | 0.0 |
| Контраст | 0.0 — 2.0 | 1.0 |
| Гамма | 0.0 — 5.0 | 2.4 |
| Яркость | -1.0 — 1.0 | 0.0 |

---

## Встроенные пресеты

| Пресет | Температура | Насыщенность | Яркость |
|---|---|---|---|
| Стандарт | 6500 K | 1.00 | 0.00 |
| Яркий | 6500 K | 1.30 | +0.20 |
| Насыщенный | 6500 K | 1.60 | 0.00 |
| Ночной | 3200 K | 0.90 | -0.10 |
| Мягкий | 5500 K | 0.80 | -0.10 |

---

## Сборка из исходников

### Требования
- devkitA64 (devkitPro)
- libnx
- switch-glm

### Команды
```bash
git clone --recurse-submodules https://github.com/Dimasick-git/Fizeau.git
cd Fizeau
make -C common -j$(nproc)
make dist -j$(nproc)
# Результат: out/Fizeau-2.9.2.zip
```

### Автоматический релиз
```bash
git tag v2.9.2
git push origin v2.9.2
# GitHub Actions соберёт и создаст релиз автоматически
```

---

## Структура проекта

```
Fizeau/
├── overlay/          # Tesla-оверлей (основной интерфейс)
│   ├── src/
│   │   ├── main.cpp      # Весь UI и логика оверлея
│   │   ├── i18n.hpp      # Строки RU/EN
│   │   └── presets.hpp   # Система пресетов
│   └── lib/libryazhahand # Tesla-библиотека (submodule)
├── common/           # Общая библиотека (CMU, конфиг, типы)
├── sysmodule/        # Системный модуль (фоновая служба)
├── application/      # NRO-приложение
└── .github/workflows/main.yml  # CI/CD
```

---

## Зависимости

| Библиотека | Назначение |
|---|---|
| [libryazhahand](https://github.com/Dimasick-git/libryazhahand) | Tesla UI (форк libultrahand) |
| [inih](https://github.com/benhoyt/inih) | Парсинг .ini файлов |
| [imgui](https://github.com/ocornut/imgui) | GUI для приложения |
| [oss-nvjpg](https://github.com/averne/oss-nvjpg) | JPEG для приложения |

---

## Основано на

- [ppkantorski/Fizeau](https://github.com/ppkantorski/Fizeau) — расширенный форк с профилями и периодами
- [averne/Fizeau](https://github.com/averne/Fizeau) — оригинальный проект

---

## Лицензия

GNU General Public License v2.0 — см. [LICENSE](LICENSE)

---

*Часть экосистемы [Ryazhenka](https://github.com/Dimasick-git) для Nintendo Switch CFW*
