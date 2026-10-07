"""Fetch each Libretro box-art index once; write a distributable JSON catalog.
Run from any directory: python flutter/gui/tool/build_boxart_catalog.py
No cover images are downloaded. Re-run only when explicitly refreshing the catalog.
"""
import json
import re
from datetime import datetime, timezone
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import urljoin, urlsplit, unquote
from urllib.request import Request, urlopen

SOURCES = {
    'gb': 'https://thumbnails.libretro.com/Nintendo%20-%20Game%20Boy/Named_Boxarts/',
    'gbc': 'https://thumbnails.libretro.com/Nintendo%20-%20Game%20Boy%20Color/Named_Boxarts/',
}

class Links(HTMLParser):
    def __init__(self):
        super().__init__()
        self.hrefs = []
    def handle_starttag(self, tag, attrs):
        if tag == 'a':
            self.hrefs.extend(v for k, v in attrs if k == 'href' and v)

def short_name(name):
    name = re.sub(r'\.(png|gbc|gb|zip)$', '', name, flags=re.I)
    while True:
        cleaned = re.sub(r'\([^()]*\)|\[[^\[\]]*\]', '', name)
        if cleaned == name:
            break
        name = cleaned
    return ' '.join(name.split())

def key(name):
    return re.sub(r'[^a-z0-9]+', ' ', short_name(name).lower().replace('é', 'e')).strip()

def main():
    entries = []
    for system, base in SOURCES.items():
        with urlopen(Request(base, headers={'User-Agent': 'GBCEmulator-boxart-catalog/1.0'}), timeout=60) as response:
            parser = Links()
            parser.feed(response.read().decode('utf-8'))
        urls = sorted({urljoin(base, href) for href in parser.hrefs if urlsplit(href).path.lower().endswith('.png')})
        urls = [url for url in urls if url.startswith(base)]
        if not urls:
            raise RuntimeError(f'No images found at {base}; existing catalog left untouched')
        for url in urls:
            filename = unquote(urlsplit(url).path.rsplit('/', 1)[-1])
            entries.append(dict(system=system, filename=filename, short_name=short_name(filename), match_key=key(filename), image_url=url))
        print(f'{system}: {len(urls)} entries')
    output = Path(__file__).resolve().parent.parent / 'assets' / 'boxart_catalog.json'
    data = dict(schema_version=1, fetched_at=datetime.now(timezone.utc).isoformat(), sources=SOURCES, entries=entries)
    output.write_text(json.dumps(data, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'Saved {len(entries)} entries to {output} ({output.stat().st_size:,} bytes)')

if __name__ == '__main__':
    main()
