#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2024, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Get some metrics on the TSDuck source code.
#
#-----------------------------------------------------------------------------

import tsbuild, sys, os, fnmatch

# A class to hold a file type.
# If description is None, the file type is to be ignored.
# Pattern can be a string, or a list of strings.
class filetype:
    # Constructor.
    def __init__(self, description=None, pattern=None, single_comment=None, open_comment=None, close_comment=None):
        self.description = description
        self.pattern = []
        self.single_comment = single_comment
        self.open_comment = open_comment
        self.close_comment = close_comment
        self.files = 0
        self.total_lines = 0
        self.code_lines = 0
        self.comment_lines = 0
        self.blank_lines = 0
        self.total_chars = 0
        self.code_chars = 0
        self.comment_chars = 0
        if isinstance(pattern, str):
            self.pattern = [pattern]
        elif isinstance(pattern, list):
            self.pattern = pattern
        elif pattern is not None:
            tsbuild.fatal_error('invalid file pattern: %s' % str(pattern))

    # Copy from another instance, reset counters.
    def copy(other):
        ftype = filetype()
        ftype.description = other.description
        ftype.pattern = other.pattern
        ftype.single_comment = other.single_comment
        ftype.open_comment = other.open_comment
        ftype.close_comment = other.close_comment
        return ftype

    # Check if a file path matches the file type.
    def match(self, path):
        name = os.path.basename(path)
        for pat in self.pattern:
            if fnmatch.fnmatch(name, pat):
                return True
        return False

    # Add results from another instance.
    def add(self, other):
        self.files += other.files
        self.total_lines += other.total_lines
        self.code_lines += other.code_lines
        self.comment_lines += other.comment_lines
        self.blank_lines += other.blank_lines
        self.total_chars += other.total_chars
        self.code_chars += other.code_chars
        self.comment_chars += other.comment_chars

    # Display width of description.
    description_header = 'File type'
    description_width = len(description_header)
    data_width = 10
    percent_width = 6

    # Display header line.
    def print_header():
        print('%-*s  %*s  %*s  %*s  %*s  %*s  %*s  %*s  %*s' %
              (filetype.description_width, filetype.description_header,
               filetype.data_width, 'Files',
               filetype.data_width, 'Blank', filetype.percent_width, '',
               filetype.data_width, 'Comment', filetype.percent_width, '',
               filetype.data_width, 'Code', filetype.percent_width, '',
               filetype.data_width, 'Total'))
        print('%s  %s  %s  %s  %s  %s  %s  %s  %s' %
              (filetype.description_width * '-',
               filetype.data_width * '-',
               filetype.data_width * '-', filetype.percent_width * ' ',
               filetype.data_width * '-', filetype.percent_width * ' ',
               filetype.data_width * '-', filetype.percent_width * ' ',
               filetype.data_width * '-'))

    # Format a percentage.
    def percent(x, max):
        return '' if max == 0 else '(%d%%)' % int((100*x)/max)

    # Display on one line.
    def print(self):
        print('%-*s  %*d  %*d  %*s  %*d  %*s  %*d  %*s  %*d' %
              (filetype.description_width, self.description,
               filetype.data_width, self.files,
               filetype.data_width, self.blank_lines,
               filetype.percent_width, filetype.percent(self.blank_lines, self.total_lines),
               filetype.data_width, self.comment_lines,
               filetype.percent_width, filetype.percent(self.comment_lines, self.total_lines),
               filetype.data_width, self.code_lines,
               filetype.percent_width, filetype.percent(self.code_lines, self.total_lines),
               filetype.data_width, self.total_lines))

    # Process one source file.
    def process_file(self, path):
        self.files += 1
        incomment = False
        with open(path, 'r') as input:
            for line in input:
                lenline = len(line)
                self.total_lines += 1
                self.total_chars += lenline
                # Analyze one source line.
                start = 0
                if lenline == 0 or line.isspace():
                    # Empty line.
                    if incomment:
                        self.comment_lines += 1
                        self.comment_chars += lenline
                    else:
                        self.blank_lines += 1
                    continue # read next line
                if incomment:
                    # Inside multiline comment.
                    start = line.find(self.close_comment)
                    if start >= 0:
                        start += len(self.close_comment)
                    if start < 0 or line[start:].isspace():
                        # Full line multiline comment.
                        self.comment_lines += 1
                        self.comment_chars += lenline
                        incomment = start < 0
                        continue # read next line
                    # There is something after the closing comment.
                    self.comment_chars += start
                    incomment = False
                # General code and/or comment line. Analyze line in range start..end.
                hascode = False
                end = lenline
                i = start
                while i < end:
                    if self.single_comment is not None and line[i:].startswith(self.single_comment):
                        # Start of single line comment.
                        self.comment_chars += end - i
                        end = i
                        break # end of line analysis
                    if self.open_comment is not None and line[i:].startswith(self.open_comment):
                        # Start of multiline comment.
                        endcomment = line.find(self.close_comment, i + len(self.open_comment))
                        if endcomment < 0:
                            # Comment continues on next lines.
                            self.comment_chars += end - i
                            incomment = True
                            end = i
                            break # end of line analysis
                        else:
                            # Comment ends on same line.
                            # Count data before comment.
                            lenbefore = i - start
                            if hascode:
                                self.code_chars += lenbefore
                            else:
                                self.comment_chars += lenbefore
                            # Count data inside comment.
                            endcomment += len(self.close_comment)
                            self.comment_chars += endcomment - i
                            # Restart line analysis after comment.
                            start = endcomment
                            i = endcomment
                            continue # next character on line
                    # Not in a comment. Non-space character outside comment means code.
                    c = line[i]
                    i = i + 1
                    if not line[i-1:i].isspace():
                        hascode = True
                        if c == '\'' or c == '"':
                            # Start of string.
                            while i < end and line[i] != c:
                                if line[i] == '\\':
                                    # ignore next character.
                                    i = i + 1
                                i = i + 1
                # Reached end of line.
                if hascode:
                    self.code_lines += 1
                    self.code_chars += end - start
                else:
                    self.comment_lines += 1
                    self.comment_chars += end - start
                        
# Results by type appear in this order.
files_total = filetype('Total')
files_order = [
    filetype('C++ code',      '*.cpp', '//', '/*', '*/'),
    filetype('C++ header',    '*.h', '//', '/*', '*/'),
    filetype('Java',          '*.java', '//', '/*', '*/'),
    filetype('Python',        '*.py', '#'),
    filetype('Ruby',          '*.rb', '#'),
    filetype('Bash shell',    ['*.sh', '*.bash', 'bash-*', 'tsconfig'], '#'),
    filetype('PowerShell',    ['*.ps1', '*.psm1'], '#'),
    filetype('Tools config',  ['.gitattributes', '.gitignore', '.clang-format', '.editorconfig', '.classpath', '.project', '*.supp', '*.rc', '*.reg'], '#'),
    filetype('Make',          ['Makefile*', '*.mk'], '#'),
    filetype('Visual Studio', ['*.sln', '*.vcxproj', '*.props']),
    filetype('Qt Creator',    ['*.pro', '*.pri'], '#'),
    filetype('Names config',  '*.names', '#'),
    filetype('YAML',          '*.yml', '#'),
    filetype('XML',           '*.xml', None, '<!--', '-->'),
    filetype('JSON',          '*.json', '#'),
    filetype('Markdown',      '*.md'),
    filetype('Text',          '*.txt'),
    filetype('Web doc',       ['*.html', '*.css', '*.js']),
    filetype('Asciidoc',      '*.adoc', '//'),
    filetype('Doxygen',       ['Doxyfile*', '*.dox'], '#'),
    filetype('Packaging',     ['*.nsi', '*.control', '*.spec', '*.perms', '*.rules', '*.pc', 'Dockerfile*'], '#'),
    filetype(None,            '.git'),
    filetype(None,            '__pycache__'),
    filetype(None,            'bin'),
    filetype(None,            'installers'),
    filetype(None,            'build'),
    filetype(None,            '*.xlsx'),
    filetype(None,            '*.pptx'),
    filetype(None,            '*.pdf'),
    filetype(None,            '*.bin'),
    filetype(None,            '*.ts'),
    filetype(None,            '*.xcf'),
    filetype(None,            '*.png'),
    filetype(None,            '*.ico'),
    filetype(None,            '*.arch-*'),
    filetype(None,            '*.user')
]

# Process all files in a directory of the project.
def process_directory(root):
    for name in os.listdir(root):
        path = root + os.sep + name
        # Find applicable type for this file.
        ftype = None
        for ft in files_order:
            if ft.match(name):
                ftype = ft
                break
        if os.path.isdir(path) and (ftype is None or ftype.description is not None):
            process_directory(path)
        elif ftype is None:
            tsbuild.error('unknown file type: %s' % path)
        elif ftype.description is None:
            pass # ignore that file
        else:
            # Process the file.
            ft = filetype.copy(ftype)
            ft.process_file(path)
            ftype.add(ft)
            files_total.add(ft)

# Main code.
if __name__ == '__main__':
    process_directory(tsbuild.repo_root())
    filetype.description_width = max(filetype.description_width, len(files_total.description))
    filetype.description_width = max(filetype.description_width, max([len(ft.description) for ft in files_order if ft.description is not None]))
    filetype.print_header()
    for ft in files_order:
        if ft.description is not None and ft.total_lines > 0:
            ft.print()
    files_total.print()
