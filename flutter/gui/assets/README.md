# Box-art catalog

`boxart_catalog.json` is a read-only JSON database bundled through pubspec.yaml.
It contains 3,160 image entries: 1,638 Game Boy and 1,522 Game Boy Color entries.
Each record preserves system, original filename, shortened title, normalized match
key and the absolute HTTPS PNG URL. Regional variants remain separate records.
The root contains the schema version, fetch timestamp and source index URLs.

Sources:
- https://thumbnails.libretro.com/Nintendo%20-%20Game%20Boy/Named_Boxarts/
- https://thumbnails.libretro.com/Nintendo%20-%20Game%20Boy%20Color/Named_Boxarts/

To explicitly refresh, run from the repository root:

```powershell
python flutter/gui/tool/build_boxart_catalog.py
```

The importer downloads each directory listing once per invocation and no images.
Normal app startup never scrapes the indexes. It loads the bundled catalog and
fetches individual covers on demand. Flutter provides an in-memory image cache;
this does not provide an offline image collection or persistent disk image cache.
The metadata catalog is distributable with the app; image rights remain with their
respective owners. Source attribution: Libretro thumbnails.

Matching strips extensions and nested ()/[] metadata, folds case, normalizes
punctuation and maps Pokemon's accented e to e. It matches full normalized titles,
not substrings, to avoid confusing sequels. .gb/.gbc or [C] distinguish consoles;
ZIP filenames without console metadata remain ambiguous. Exact original titles
win, then regional matches, with USA/Europe/Japan as deterministic fallbacks.
Unmatched titles and failed network images show placeholders. Renamed ROMs may
need filenames closer to the catalog; ZIP contents are not inspected for matching.
