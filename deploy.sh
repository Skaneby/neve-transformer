#!/bin/bash
# Deploy signed app bundle to Google Drive with date-time stamp
SRC="$1"
DEPLOY_DIR="$HOME/Library/CloudStorage/GoogleDrive-johan.skaneby@gmail.com/My Drive/Egna projekt/herrstrom"
STAMP=$(date +%Y-%m-%d_%H-%M-%S)
APP_NAME="Neve Transformer_${STAMP}.app"
HELPER_NAME="Open Neve Transformer.command"

mkdir -p "$DEPLOY_DIR"
cp -R "$SRC" "$DEPLOY_DIR/$APP_NAME"
# Strip quarantine so macOS Gatekeeper doesn't block it as "damaged" (Google Drive desktop sync)
xattr -cr "$DEPLOY_DIR/$APP_NAME"

# Create a double-clickable helper script for recipients who downloaded via browser.
# Browser downloads get quarantine re-added by macOS; running this script re-signs
# the app locally (ad-hoc) which is enough for macOS Sonoma/Sequoia to accept it.
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

echo "Signing and opening: $(basename "$APP")"
xattr -cr "$APP"
codesign --force --deep -s - "$APP"
open "$APP"
echo "Done!"
EOF

chmod +x "$DEPLOY_DIR/$HELPER_NAME"
# Strip quarantine from the helper itself so it opens with a simple "Are you sure?" prompt
xattr -cr "$DEPLOY_DIR/$HELPER_NAME"

echo "Deployed: $APP_NAME"
echo "Helper:   $HELPER_NAME"
