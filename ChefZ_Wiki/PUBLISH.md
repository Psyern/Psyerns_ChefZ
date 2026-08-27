# Publishing this wiki to GitHub

These pages are the source of truth and live in the main repository under
`ChefZ_Wiki/`. GitHub's wiki is a **separate** git repository that mirrors them.

## One-time setup — this part needs you

GitHub does not create `Psyerns_ChefZ.wiki.git` until the wiki has been enabled and
given a first page through the web interface. Until then a push fails with
`Repository not found`, which is what happens today.

1. Open <https://github.com/Psyern/Psyerns_ChefZ/settings> and make sure **Wikis**
   is ticked under *Features*.
2. Open <https://github.com/Psyern/Psyerns_ChefZ/wiki> and click **Create the first
   page**. Any content will do — the next step overwrites it.
3. Save the page.

The wiki repository now exists.

## Publishing

From the repository root:

```bash
node ChefZ_Wiki/publish.mjs
```

The script clones the wiki repository into a temporary directory, replaces its
contents with `ChefZ_Wiki/`, commits and pushes. It refuses to run if the wiki
repository does not exist yet, and it says so rather than failing obscurely.

To see what would happen without touching anything:

```bash
node ChefZ_Wiki/publish.mjs --dry-run
```

## How the pages map

GitHub wikis are flat: the file name is the page name and `-` becomes a space in
the title. `Recipe-Reference.md` becomes the page *Recipe Reference* at
`/wiki/Recipe-Reference`.

Two files are special and are not pages:

- `_Sidebar.md` — the navigation shown beside every page
- `_Footer.md` — the line shown below every page

`PUBLISH.md` and `publish.mjs` are excluded from the published wiki; they are
tooling, not content.

## Editing

Edit here, in the main repository, and publish. Edits made in GitHub's web editor
live only in the wiki repository and are overwritten by the next publish — the
script warns when it finds commits in the wiki that did not come from here.
