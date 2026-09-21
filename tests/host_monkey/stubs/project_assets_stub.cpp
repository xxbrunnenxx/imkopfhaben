// Host-Stub fuer project_assets: auf dem Pi gibt es keine eingebetteten
// Bild-Assets. Der Monkey-Test prueft Navigations-/Zustandslogik, keine
// Darstellung -- Icons duerfen deshalb nullptr sein. Der Coordinator setzt
// die Zeiger nur in ListItem-Felder, die im Test nie dereferenziert werden.
#include "project_assets.h"

namespace project_assets {

const EmbeddedImageAsset* GetLogo(EmbeddedLogoId) { return nullptr; }
const EmbeddedImageAsset* GetIcon(EmbeddedIconId) { return nullptr; }
const EmbeddedImageAsset* GetFooterIcon(EmbeddedFooterIconId) { return nullptr; }
const EmbeddedImageAsset* GetImage(EmbeddedImageId) { return nullptr; }

}  // namespace project_assets
