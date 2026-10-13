# BeatCanvas – מטרונום ויזואלי ל‑Logic Pro (Audio Unit)

פלאגין אפקט (AU) שמעביר את האודיו כמו שהוא, ובמקום זה מציג אנימציות שמסונכרנות לפעימות הפרויקט.
נבנה עם **JUCE 8 (C++)**.

## למה הטכנולוגיה הזו
| שאלה | החלטה | סיבה |
|---|---|---|
| איך מקבלים AU? | JUCE → `AU` + `VST3` + `Standalone` מאותו קוד | Logic טוען רק AU. ה‑Standalone מאפשר לפתח ולראות אנימציות בלי לפתוח Logic |
| סוג הפלאגין | **Effect** (`aufx`), לא MIDI FX | נטען על כל ערוץ, מקבל Playhead (טמפו/מיקום), ומאפשר גם קליק קולי |
| רינדור | `juce::Graphics` (CoreGraphics), מסונכרן ל‑VBlank של המסך | OpenGL deprecated ב‑macOS וידוע כבעייתי בתוך חלונות פלאגין של Logic; Graphics יציב ומספיק ל‑60fps |
| סנכרון | ה‑audio thread מפרסם `(ppq, bpm, זמן)` בכל בלוק (seqlock, ללא נעילות). ה‑UI לא סופר פעימות בעצמו אלא מבצע אקסטרפולציה + תיקון עדין (PLL) | אין סטייה מצטברת (המיקום תמיד מגיע מה‑host), ואין ריצוד מג'יטר של בלוקים |
| ביצועים | אין הקצאות בלולאת האודיו; הציור רק כשהחלון פתוח | כמעט אפס עומס על ה‑CPU של האודיו |

## התקנה והפעלה

### אפשרות א׳ – יש לך Mac
```bash
brew install cmake           # פעם אחת; וגם: xcode-select --install
./scripts/build_install_mac.sh
```
הסקריפט בונה AU אוניברסלי (Apple Silicon + Intel), מריץ את בדיקת הסנכרון, חותם (ad‑hoc), מתקין ל‑`~/Library/Audio/Plug-Ins/Components` ומריץ `auval` – אותה בדיקה ש‑Logic עושה.

ב‑Logic: חריץ Audio FX ← **Audio Units ← Zevik ← BeatCanvas**.
אם לא מופיע: `Logic Pro ‣ Settings ‣ Plug-in Manager ‣ Reset & Rescan Selection`.

### אפשרות ב׳ – בלי Mac (בנייה בענן)
דחוף את התיקייה ל‑GitHub. ה‑workflow `.github/workflows/build-macos.yml` בונה ב‑macOS, מריץ `auval`, ומפיק `BeatCanvas-macOS.zip` תחת Actions ← Artifacts.
אחרי ההורדה:
```bash
xattr -cr BeatCanvas.component && cp -R BeatCanvas.component ~/Library/Audio/Plug-Ins/Components/
```

### פיתוח ב‑Windows/Linux
בונים `VST3` + `Standalone` (AU קיים רק ב‑macOS): `cmake -B build && cmake --build build --config Release`.

## שימוש
* **Sync** – `Host Sync` (עוקב אחרי Logic) או `Manual BPM`.
* **Run when stopped** – האנימציה ממשיכה לרוץ בטמפו הפרויקט גם כש‑Logic עצור.
* **First beat / Other beats** – אנימציה שונה לפעימה הראשונה בתיבה ולשאר. **Surprise me** בוחר אקראית.
* **Palette / Style / Intensity / Speed** – צבעים, צורת הפולס (Smooth/Punchy/Elastic/Stepped), עוצמה, מהירות.
* **Visual offset** – פיצוי השהיית אודיו: ערך שלילי = הוויזואל מאוחר יותר.
* **Follow host meter** – משקל מה‑host (למשל 6/8, 7/8). כבוי = אפשר לקבוע פעימות ליחידה ידנית.
* **Audible click** – קליק מדויק לדגימה (הדגשה בפעימה ראשונה).
* **Preset** – 9 פריסטים מובנים, ו‑`Save preset...` שומר את כל המצב לקובץ `Documents/BeatCanvas/Presets/<שם>.bcpreset` (אפשר להעתיק בין מחשבים). המצב נשמר גם בתוך פרויקט Logic.
* הכפתור `...` בפינה מסתיר/מציג את הפאנל. החלון ניתן לשינוי גודל.

## הוספת אנימציה חדשה
צור קובץ `Source/Animations/MyThing.h` – זה הכל. CMake מזהה אותו אוטומטית (הרץ configure מחדש).
```cpp
#pragma once
#include "../ParticleSystem.h"          // או ../Animation.h בלבד

class MyThing : public Animation
{
public:
    void onBeat (const FrameContext& c) override { /* נקרא בתחילת כל פעימה; c.beat.downbeat() */ }
    void paint (FrameContext& c) override
    {
        c.background (c.pulse * 0.2f);                       // רקע + הבהוב
        auto r = c.unit() * (0.2f + 0.1f * c.pulse);         // pulse: 1 בפעימה ← 0
        c.g.setColour (c.col (0));                           // צבעי הפלטה: col(0..3) / grad(0..1)
        c.g.fillEllipse (c.centre().x - r, c.centre().y - r, 2 * r, 2 * r);
    }
};
BC_REGISTER_ANIMATION (MyThing, "mything", "My Thing", "Category")
```
מה יש ב‑`FrameContext`: `beat` (phase, beatIndex, beatPos…), `pulse`, `accent` (1 בפעימה ראשונה), `time` ו‑`dt` (מוכפלים ב‑Speed), `intensity`, `pal`. `ParticleSystem` נותן מאגר חלקיקים מוכן.
ה‑id נשמר בפריסטים – אל תשנה אותו אחרי שהשתמשת בו.

## מבנה
```
Source/BeatClock.h          סנכרון: mailbox + PLL (C++ טהור, נבדק ב-Tests/)
Source/PluginProcessor.*    audio thread, פרמטרים, קליק
Source/PluginEditor.*       קנבס אנימציה + פאנל בקרה + look&feel
Source/Animation.h          ממשק Animation, FrameContext, רישום
Source/Animations/*.h       הספרייה (14 אנימציות)
Tests/ClockTest.cpp         מדמה 20 דקות עם ג'יטר: שגיאה מרבית ממוצעת, שינוי טמפו, seek, משקלים
```

## רישיון
JUCE הוא AGPLv3 / מסחרי. שימוש אישי ופרויקט קוד פתוח – בסדר; הפצה מסחרית סגורה דורשת רישיון JUCE.
