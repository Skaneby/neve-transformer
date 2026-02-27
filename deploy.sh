#!/bin/bash
# Deploy signed app bundle + installer to Google Drive with date-time stamp
SRC="$1"
DEPLOY_DIR="$HOME/Library/CloudStorage/GoogleDrive-johan.skaneby@gmail.com/My Drive/Egna projekt/herrstrom"
STAMP=$(date +%Y-%m-%d_%H-%M-%S)
APP_NAME="Neve Transformer_${STAMP}.app"
PKG_NAME="Neve Transformer_${STAMP}.pkg"
HELPER_NAME="Open Neve Transformer.command"

mkdir -p "$DEPLOY_DIR"
cp -R "$SRC" "$DEPLOY_DIR/$APP_NAME"
# Strip quarantine so macOS Gatekeeper doesn't block it as "damaged" (Google Drive desktop sync)
xattr -cr "$DEPLOY_DIR/$APP_NAME"

# Build .pkg installer — installs app to /Applications.
# Unlike a bare .app, a .pkg shows "unidentified developer" in Security & Privacy
# where the user can click "Open Anyway". The installed app gets no quarantine
# attribute, so it opens without further issues on all macOS versions.
pkgbuild \
    --install-location /Applications \
    --component "$DEPLOY_DIR/$APP_NAME" \
    --identifier "com.herrstrom.neve-transformer" \
    --version "1.0.0" \
    "$DEPLOY_DIR/$PKG_NAME"

# Keep helper script as fallback for users who prefer the .app directly.
cat > "$DEPLOY_DIR/$HELPER_NAME" << 'EOF'
#!/bin/bash
# Neve Transformer — First-time opener
# Run this once if macOS says "can't be opened" when double-clicking the app.

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APP=$(find "$DIR" -maxdepth 1 -name "Neve Transformer*.app" | sort | tail -1)

if [ -z "$APP" ]; then
    echo "Could not find Neve Transformer.app in the same folder as this script."
    read -p "Press Enter to close..."
    exit 1
fi

echo "Removing quarantine from: $(basename "$APP")"
# Strip all extended attributes (quarantine etc) — app is already correctly
# signed at build time, do NOT re-sign here as that breaks launch on macOS 12+
xattr -cr "$APP"
open "$APP"
echo "Done!"
EOF

chmod +x "$DEPLOY_DIR/$HELPER_NAME"
# Strip quarantine from the helper itself so it opens with a simple "Are you sure?" prompt
xattr -cr "$DEPLOY_DIR/$HELPER_NAME"

echo "Deployed: $APP_NAME"
echo "Package:  $PKG_NAME"
echo "Helper:   $HELPER_NAME"
