# Converts the project Wiki to html and adds it as manual.
#
# Adapted from https://gist.github.com/mrexmelle/659abc02ae1295d60647

import os
import subprocess
import sys
from bs4 import BeautifulSoup

wikiDir = '/tmp/PuRestJson.wiki/'
exportDir = 'manual/'

if len(sys.argv) > 1:
    exportDir = sys.argv[1]

if len(sys.argv) > 2:
    wikiDir = sys.argv[2]

print 'Input directory: ', wikiDir
print 'Output directory: ', exportDir

# rename all files containing '[' and ']' in names,
# because Windows does not like those.
for f in os.listdir(wikiDir):
    cleaned = f.replace('[', '').replace(']', '').replace(':', '')
    if f != cleaned:
        os.rename(wikiDir + f, wikiDir + cleaned)

# convert md files one-by-one.
for f in os.listdir(wikiDir):
    if f.endswith('.md'):
        print 'Converting: ', f
        baseFile = os.path.splitext(os.path.basename(f))[0];
        htmlFile = baseFile + '.html'
        subprocess.call(['grip', wikiDir + f, '--export', '--no-inline', 
            exportDir + htmlFile])
        # edit links to css, images and other pages.
        htmlDoc = open(exportDir + htmlFile)
        soup = BeautifulSoup(htmlDoc, 'lxml')
        for s in soup.findAll('link'):
            s.extract()
        css = soup.new_tag('link')
        css.attrs['rel'] = 'stylesheet'
        css.attrs['href'] = 'style.css'
        soup.head.append(css)
        # internal links.
        for a in soup.findAll('a'):
            if a['href'].startswith('https://github.com/residuum/PuRestJson/wiki/'):
                a['href'] = a['href']\
                        .replace('https://github.com/residuum/PuRestJson/wiki/', '')\
                        .replace('%5B', '')\
                        .replace('%5D', '')\
                        .replace(':', '') + '.html'
        # images with link to itself.
        for img in soup.findAll('img'):
            if img['src'].startswith('https://camo.githubusercontent.com'):
                img['src'] = img['data-canonical-src']\
                        .replace('https://raw.github.com/residuum/PuRestJson/master/manual/','')
                del img['data-canonical-src']
                a = img.parent
                del a['href']
        # write changes back to file
        htmlDoc.close()
        html = soup.prettify('utf-8')
        with open(exportDir + htmlFile, 'w') as edited:
            edited.write(html)

os.rename(exportDir + "Home.html", exportDir + "index.html")
