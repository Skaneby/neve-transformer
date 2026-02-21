#!/bin/bash
# Deploy signed app bundle to Google Drive with date-time stamp
SRC="$1"
DEPLOY_DIR="$HOME/Library/CloudStorage/GoogleDrive-johan.skaneby@gmail.com/My Drive/Egna projekt/herrstrom"
STAMP=$(date +%Y-%m-%d_%H-%M-%S)

mkdir -p "$DEPLOY_DIR"
cp -R "$SRC" "$DEPLOY_DIR/Neve Transformer_${STAMP}.app"
echo "Deployed: Neve Transformer_${STAMP}.app"
