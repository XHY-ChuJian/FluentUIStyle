#pragma once

#include <QString>

namespace GalleryCrashDump
{
// Installs the process-wide Windows crash handlers and returns the directory
// where dump files will be written. Other platforms return an empty string.
QString install();
}
