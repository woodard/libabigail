#!/usr/bin/env python
# SPDX-License-Identifier: LGPL-2.0-or-later
#
# -*- mode: python -*-
#
# This python program has been copied from
# https://github.com/GNOME/gnet/blob/master/gen-changelog.py
#
# It has been authored by Edward Hervey, of GStreamer fame.  I have
# asked his permission to copy it and re-use it here, as part of the
# Libabigail project.  He granted me the permission to distribute the
# program under the terms of the GNU Lesser Public License as
# published by the Free Software Foundaion; either version 2, or (at
# your option) any later version.
#

import sys
import subprocess
import re

# Makes a GNU-Style ChangeLog from a git repository
# Handles git-svn repositories also

# Arguments : same as for git log
release_refs={}

def process_commit(lines, files):
    # DATE NAME
    # BLANK LINE
    # Subject
    # BLANK LINE
    # ...
    # FILES
    fileincommit = False
    lines = [x.strip() for x in lines if x.strip() and not x.startswith('git-svn-id')]
    files = [x.strip() for x in files if x.strip()]
    for l in lines:
        if l.startswith('* '):
            fileincommit = True
            break

    top_line = lines[0]
    subject_line_index = 0 if lines[1].startswith('*') else 1;
    first_cl_body_line_index = 0;

    for i in range(1, len(lines)):
        if lines[i].startswith('*'):
            first_cl_body_line_index = i
            break;

    # Clean up top line of ChangeLog entry:
    fields = top_line.split(' ')

    # 1. remove the time and timezone stuff in "2008-05-13 07:10:28 +0000  name"
    if fields[2].startswith('+') or fields[2].startswith('-'):
      del fields[2]
    if fields[1][2] == ':' and fields[1][5] == ':':
      del fields[1]

    # 2. munge at least my @src.gnome.org e-mail address...
    if fields[-1] == '<tpm@src.gnome.org>':
      fields[-1] = '<tim@centricular.net>'

    top_line = ' '.join(fields)
    print(top_line.strip())
    print()

    if subject_line_index > 0:
        print('\t%s' % lines[subject_line_index].strip())

    if not fileincommit:
        for f in files:
            print('\t* %s:' % f.strip())
        print()

    if first_cl_body_line_index > 0:
        for l in lines[first_cl_body_line_index:]:
            if l.startswith('Signed-off-by:'):
                continue
            print('\t%s' % l.strip())
        print()

def output_commits():
    cmd = ['git', 'log', '--pretty=format:--START-COMMIT--%H%n%ai  %an <%ae>%n%n%s%n%b%n--END-COMMIT--',
           '--date=short', '--name-only']

    start_tag = find_start_tag()

    if start_tag is None:
        cmd.extend(sys.argv[1:])
    else:
        cmd.extend(["%s..HEAD" % (start_tag)])

    p = subprocess.Popen(args=cmd, shell=False,
                         stdout=subprocess.PIPE,
                         text=True)
    buf = []
    files = []
    filemode = False
    for lin in p.stdout.readlines():
        if lin.startswith("--START-COMMIT--"):
            if buf != []:
                process_commit(buf, files)
            hash = lin[16:].strip()
            try:
                rel = release_refs[hash]
                print("=== release %d.%d.%d ===\n" % (int(rel[0]), int(rel[1]), int(rel[2])))
            except:
                pass
            buf = []
            files = []
            filemode = False
        elif lin.startswith("--END-COMMIT--"):
            filemode = True
        elif filemode == True:
            files.append(lin)
        else:
            buf.append(lin)
    if buf != []:
        process_commit(buf, files)

def get_rel_tags():
    # Populate the release_refs dict with the tags for previous releases
    reltagre = re.compile("^([a-z0-9]{40}) refs\/tags\/GNET-([0-9]+)[-_.]([0-9]+)[-_.]([0-9]+)")

    cmd = ['git', 'show-ref', '--tags', '--dereference']
    p = subprocess.Popen(args=cmd, shell=False,
                         stdout=subprocess.PIPE,
                         text=True)
    for lin in p.stdout:
       match = reltagre.search(lin)
       if match:
           (sha, maj, min, nano) = match.groups()
           release_refs[sha] = (maj, min, nano)

def find_start_tag():
    starttagre = re.compile("^([a-z0-9]{40}) refs\/tags\/CHANGELOG_START")
    cmd = ['git', 'show-ref', '--tags']
    p = subprocess.Popen(args=cmd, shell=False,
                         stdout=subprocess.PIPE,
                         text=True)
    for lin in p.stdout:
       match = starttagre.search(lin)
       if match:
           return match.group(1)
    return None

if __name__ == "__main__":
    get_rel_tags()
    output_commits()
