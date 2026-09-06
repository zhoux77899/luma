# User guide site is assembled from Release tags; latest tracks main

The User guide source stays one tree on main (`docs/user-manual/`). The User guide site rebuilds from each GitHub Release tag plus current main so historical copies never enter the firmware repo. `latest` is the main-branch handbook, not GitHub Latest. A tag without `docs/user-manual/` still appears in the version switcher as a stub. The site shell is Vite + React with beUI chrome, not VitePress, so cover and navigation can use the React registry while guide bodies stay quiet Markdown.
