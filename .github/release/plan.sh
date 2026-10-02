#!/bin/bash
# Prints the release plan as GitHub Actions outputs: the applications due and their CMake targets.
# An application is due when the version in its version.txt has no tag yet. DRY_RUN=true plans
# every application, tagged or not.

set -euo pipefail

# name, folder, files shipped beside the executable
apps=(
    "MillScene Showcase/MillScene"
    "TextEngine Showcase/TextEngine"
    "WhatsClip Tools/WhatsClip"
    "Themes Tools/Themes"
    "PascalScripts Tools/PascalScripts Language.cfg"
)

plan='[]'
for entry in "${apps[@]}"; do
    read -r app dir files <<< "$entry"
    version=$(sed '1s/^\xEF\xBB\xBF//' "$dir/version.txt" | tr -d '[:space:]')
    if ! [[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "$dir/version.txt holds '$version' - one version, major.minor.patch, is expected" >&2
        exit 1
    fi
    tag="$app-v$version"
    if [ "${DRY_RUN:-false}" != true ]; then
        found=0
        git ls-remote --exit-code --tags origin "refs/tags/$tag" > /dev/null || found=$?
        case $found in
            0) continue ;;
            2) ;;
            *) echo "the tags of origin could not be read" >&2; exit 1 ;;
        esac
    fi
    plan=$(jq -c --arg app "$app" --arg dir "$dir" --arg version "$version" --arg tag "$tag" \
        --arg files "$files" \
        '. + [{ app: $app, dir: $dir, version: $version, tag: $tag,
                files: ($files | split(" ") | map(select(. != ""))) }]' <<< "$plan")
done

echo "apps=$plan"
echo "targets=$(jq -r 'map(.app) | join(" ")' <<< "$plan")"
