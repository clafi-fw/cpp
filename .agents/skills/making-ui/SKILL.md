---
name: making-ui
description: Apply ClaFi UI conventions when building or reviewing interfaces, including messages that name files or folders or report platform errors.
---

# Making UI

## File and folder links

When a UI message names a file or folder, make its path clickable with
`AppContext::fileLink`. Clicking the path opens the file or folder it names.

If the target is missing, show the full path. Make the deepest existing folder in that path
clickable and leave the remaining text plain.

## Platform errors

Capture the original platform error as soon as an operation fails, before cleanup or another
call can overwrite it. Carry it through any background work to the UI.

Tell the user which operation failed and show the platform's own error message. Include its
error category and numeric code when available, for example `Access is denied. (system 5)`.
