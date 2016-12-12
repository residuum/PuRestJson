# converts the Wiki to generate the manual
#
# copied and adapted from https://gist.github.com/mrexmelle/659abc02ae1295d60647

import getopt
import re, shutil, tempfile
import os
import subprocess
import sys
from bs4 import BeautifulSoup

# http://stackoverflow.com/questions/4427542/how-to-do-sed-like-text-replace-with-python
def sed_inplace(filename, pattern, repl):
    '''
    Perform the pure-Python equivalent of in-place `sed` substitution: e.g.,
    `sed -i -e 's/'${pattern}'/'${repl}' "${filename}"`.
    '''
    # For efficiency, precompile the passed regular expression.
    pattern_compiled = re.compile(pattern)

    # For portability, NamedTemporaryFile() defaults to mode "w+b" (i.e., binary
    # writing with updating). This is usually a good thing. In this case,
    # however, binary writing imposes non-trivial encoding constraints trivially
    # resolved by switching to text writing. Let's do that.
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as tmp_file:
        with open(filename) as src_file:
            for line in src_file:
                tmp_file.write(pattern_compiled.sub(repl, line))

    # Overwrite the original file with the munged temporary file in a
    # manner preserving file attributes (e.g., permissions).
    shutil.copystat(filename, tmp_file.name)
    shutil.move(tmp_file.name, filename)


wikiDir='/tmp/PuRestJson.wiki/'
exportDir='manual/'
print 'Preparing directory: ', wikiDir

# convert md files one-by-one
for f in os.listdir(wikiDir):
    if f.endswith('.md'):
        print 'Converting: ', f
        baseFile=os.path.splitext(os.path.basename(f))[0];
        htmlFile=baseFile + '.html'
        subprocess.call(['grip', wikiDir + f, '--export', '--no-inline', exportDir + htmlFile])
        p=re.compile(exportDir)
        sed_inplace(exportDir + htmlFile, p, '')
        # edit links to css, images and other pages
        htmlDoc = open(exportDir + htmlFile)
        soup = BeautifulSoup(htmlDoc, 'lxml')
        for s in soup.findAll('link'):
            s.extract()
        css = soup.new_tag('link')
        css.attrs['rel'] = 'stylesheet'
        css.attrs['href'] = 'style.css'
        soup.head.append(css)
        for a in soup.findAll('a'):
            if a['href'].startswith('https://github.com/residuum/PuRestJson/wiki/'):
                a['href'] = a['href'].replace('https://github.com/residuum/PuRestJson/wiki/', '')+'.html'
        for img in soup.findAll('img'):
            if img['src'].startswith('https://camo.githubusercontent.com'):
                img['src'] = img['data-canonical-src'].replace('https://raw.github.com/residuum/PuRestJson/master/','')
        # write changes back to file
        htmlDoc.close()
        html = soup.prettify('utf-8')
        with open(exportDir + htmlFile, 'w') as edited:
            edited.write(html)

os.rename(exportDir + "Home.html", exportDir + "index.html")
