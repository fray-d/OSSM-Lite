#!/usr/bin/env bash
# Rebuilds <BUCKET>/registry.json from the bucket's actual contents and uploads
# it atomically. The registry is derived state: this script never reads the old
# registry.json, so a corrupt, empty, or missing one cannot propagate. Safe to
# run at any time; the last run to finish always converges on ground truth.
#
# Required env:
#   SUPABASE_ACCESS_TOKEN  - platform token (sbp_...) or project service key
#   SUPABASE_PROJECT_ID    - project ref
set -euo pipefail

BUCKET="ossm-firmware"
# Release channels are published by the version/release workflows and are not
# dev branches; the web flasher's Development dropdown must not list them.
PROTECTED_CHANNELS=("master" "alpha" "production")
BASE="https://${SUPABASE_PROJECT_ID}.supabase.co/storage/v1"
LIST_LIMIT=1000

# The storage REST API needs a project JWT. CI stores a platform access token
# (sbp_...), so exchange it for the service_role key the same way the CLI does.
if [[ "${SUPABASE_ACCESS_TOKEN}" == sbp_* ]]; then
  STORAGE_KEY=$(curl -sf "https://api.supabase.com/v1/projects/${SUPABASE_PROJECT_ID}/api-keys?reveal=true" \
      -H "Authorization: Bearer ${SUPABASE_ACCESS_TOKEN}" \
    | jq -re '.[] | select(.name == "service_role") | .api_key')
else
  STORAGE_KEY="${SUPABASE_ACCESS_TOKEN}"
fi

# One-level listing. Prefix travels in the JSON body, so URL-encoded branch
# names (kareem%2Fmy-branch) need no extra escaping. Folders have "id": null.
list() {
  curl -sf -X POST "${BASE}/object/list/${BUCKET}" \
    -H "Authorization: Bearer ${STORAGE_KEY}" \
    -H "apikey: ${STORAGE_KEY}" \
    -H "Content-Type: application/json" \
    -d "{\"prefix\":\"$1\",\"limit\":${LIST_LIMIT},\"offset\":0,\"sortBy\":{\"column\":\"name\",\"order\":\"asc\"}}"
}

# A listing that hits the limit means entries were silently dropped; a registry
# built from it would look complete while missing branches. Refuse instead.
assert_not_truncated() {
  local count
  count=$(echo "$1" | jq 'length')
  if [ "$count" -ge "$LIST_LIMIT" ]; then
    echo "ERROR: listing for prefix '$2' hit the ${LIST_LIMIT}-entry cap; refusing to build a truncated registry" >&2
    exit 1
  fi
}

has_manifest() {
  echo "$1" | jq -e 'map(select(.id != null and .name == "manifest.json")) | length > 0' > /dev/null
}

is_protected() {
  local name="$1" p
  for p in "${PROTECTED_CHANNELS[@]}"; do
    [ "$name" = "$p" ] && return 0
  done
  return 1
}

TOP=$(list "")
assert_not_truncated "$TOP" ""

REGISTRY='{}'
while IFS= read -r BRANCH; do
  [ -n "$BRANCH" ] || continue
  is_protected "$BRANCH" && continue
  ENTRIES=$(list "${BRANCH}/")
  assert_not_truncated "$ENTRIES" "${BRANCH}/"
  has_manifest "$ENTRIES" || continue

  # Archived commits are subfolders holding their own manifest.json. Sort by
  # the manifest's created_at so the array keeps the old append order
  # (oldest -> newest), which is what the flasher's commit dropdown expects.
  PAIRS='[]'
  while IFS= read -r SUB; do
    [ -n "$SUB" ] || continue
    SUBENTRIES=$(list "${BRANCH}/${SUB}/")
    if has_manifest "$SUBENTRIES"; then
      CREATED=$(echo "$SUBENTRIES" | jq -r '.[] | select(.id != null and .name == "manifest.json") | .created_at')
      PAIRS=$(echo "$PAIRS" | jq --arg c "$SUB" --arg t "$CREATED" '. + [{c:$c, t:$t}]')
    fi
  done < <(echo "$ENTRIES" | jq -r '.[] | select(.id == null) | .name')

  COMMITS=$(echo "$PAIRS" | jq 'sort_by(.t) | map(.c)')
  REGISTRY=$(echo "$REGISTRY" | jq --arg b "$BRANCH" --argjson c "$COMMITS" '. + {($b): $c}')
done < <(echo "$TOP" | jq -r '.[] | select(.id == null) | .name')

# Never upload anything that is not a JSON object. This inverts the failure
# mode that caused the 2026-07 outage: worst case is now a red CI job with the
# previous registry left intact, not a silently poisoned bucket.
echo "$REGISTRY" | jq -e 'type == "object"' > /dev/null
echo "$REGISTRY" | jq . > ./registry.json
[ -s ./registry.json ]

# x-upsert replaces the object in one call. No delete-then-upload window, so a
# failure anywhere in this script leaves the previous registry untouched and
# readers never see a 404.
curl -sf -X POST "${BASE}/object/${BUCKET}/registry.json" \
  -H "Authorization: Bearer ${STORAGE_KEY}" \
  -H "apikey: ${STORAGE_KEY}" \
  -H "Content-Type: application/json" \
  -H "x-upsert: true" \
  -H "cache-control: no-cache" \
  --data-binary @./registry.json > /dev/null

echo "registry.json rebuilt for ${BUCKET}: $(jq -c 'keys' ./registry.json)"
