#pragma once

enum class AppUiLanguage
{
    Zh_CN,
    En_US,
    FollowSystem
};

namespace AppLanguage
{
/** Stored preference (may include FollowSystem). */
AppUiLanguage savedUiLanguage();

/** Language used to load translator at startup: only Zh_CN or En_US. */
AppUiLanguage effectiveUiLanguage();

void saveUiLanguage(AppUiLanguage lang);

/** Install/remove translator for @a lang. Call only with Zh_CN or En_US (e.g. from main() at startup). */
void applyTranslator(AppUiLanguage lang);

/** Start a new instance with the same executable and arguments, then quit this process. Call after @c saveUiLanguage / @c QSettings::sync. */
bool restartApplication();
} // namespace AppLanguage
