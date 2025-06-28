#!/usr/bin/env python
#-----------------------------------------------------------------------------
#
#  TSDuck - The MPEG Transport Stream Toolkit
#  Copyright (c) 2005-2025, Thierry Lelegard
#  BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
#  Get some metrics on the TSDuck source code.
#
#-----------------------------------------------------------------------------

import tsbuild, sys, os, fnmatch

# A class to hold a file type.
# If description is None, the file type is to be ignored.
# Pattern can be a string or a list of strings.
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

    # Check if a file path matches one of the file types.
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
    percent_width = 9

    # Format a percentage.
    def percent(x, max):
        return '' if max == 0 else '%d%%' % int((100*x)/max)

    # Display header for lines of code.
    def print_lines_header():
        print('| %-*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |' %
              (filetype.description_width, filetype.description_header,
               filetype.data_width, 'Files',
               filetype.data_width, 'Blank', filetype.percent_width, 'Blank %',
               filetype.data_width, 'Comment', filetype.percent_width, 'Comment %',
               filetype.data_width, 'Code', filetype.percent_width, 'Code %',
               filetype.data_width, 'Total'))
        print('| %s | %s: | %s: | %s: | %s: | %s: | %s: | %s: | %s: |' %
              (filetype.description_width * '-',
               (filetype.data_width - 1) * '-',
               (filetype.data_width - 1) * '-', (filetype.percent_width - 1) * '-',
               (filetype.data_width - 1) * '-', (filetype.percent_width - 1) * '-',
               (filetype.data_width - 1) * '-', (filetype.percent_width - 1) * '-',
               (filetype.data_width - 1) * '-'))

    # Display lines of code on one line.
    def print_lines(self):
        print('| %-*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s | %*s |' %
              (filetype.description_width, self.description,
               filetype.data_width, f"{self.files:,}",
               filetype.data_width, f"{self.blank_lines:,}",
               filetype.percent_width, filetype.percent(self.blank_lines, self.total_lines),
               filetype.data_width, f"{self.comment_lines:,}",
               filetype.percent_width, filetype.percent(self.comment_lines, self.total_lines),
               filetype.data_width, f"{self.code_lines:,}",
               filetype.percent_width, filetype.percent(self.code_lines, self.total_lines),
               filetype.data_width, f"{self.total_lines:,}"))

    # Display header for characters of code.
    def print_chars_header():
        print('| %-*s | %*s | %*s | %*s | %*s | %*s | %*s |' %
              (filetype.description_width, filetype.description_header,
               filetype.data_width, 'Files',
               filetype.data_width, 'Comment', filetype.percent_width, 'Comment %',
               filetype.data_width, 'Code', filetype.percent_width, 'Code %',
               filetype.data_width, 'Total'))
        print('| %s | %s: | %s: | %s: | %s: | %s: | %s: |' %
              (filetype.description_width * '-',
               (filetype.data_width - 1) * '-',
               (filetype.data_width - 1) * '-', (filetype.percent_width - 1) * '-',
               (filetype.data_width - 1) * '-', (filetype.percent_width - 1) * '-',
               (filetype.data_width - 1) * '-'))

    # Display characters of code on one line.
    def print_chars(self):
        print('| %-*s | %*s | %*s | %*s | %*s | %*s | %*s |' %
              (filetype.description_width, self.description,
               filetype.data_width, f"{self.files:,}",
               filetype.data_width, f"{self.comment_chars:,}",
               filetype.percent_width, filetype.percent(self.comment_chars, self.total_chars),
               filetype.data_width, f"{self.code_chars:,}",
               filetype.percent_width, filetype.percent(self.code_chars, self.total_chars),
               filetype.data_width, f"{self.total_chars:,}"))

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
# Files and directories with None as description are ignored.
files_order = [
    filetype('C++ code',      '*.cpp', '//', '/*', '*/'),
    filetype('C++ header',    '*.h', '//', '/*', '*/'),
    filetype('Java',          '*.java', '//', '/*', '*/'),
    filetype('Python',        '*.py', '#'),
    filetype('Ruby',          '*.rb', '#'),
    filetype('Bash shell',    ['*.sh', '*.bash', 'bash-*', 'tsconfig'], '#'),
    filetype('PowerShell',    ['*.ps1', '*.psm1'], '#'),
    filetype('Tools config',  ['.gitattributes', '.gitignore', '.clang-format', '.editorconfig', '.classpath', '.project',
                               '*.supp', '*.rc', '*.reg'], '#'),
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
    filetype(None,            '.*.adoc'),
    filetype('Asciidoc',      '*.adoc', '//'),
    filetype('Doxygen',       ['Doxyfile*', '*.dox'], '#'),
    filetype('Packaging',     ['*.nsi', '*.control', '*.spec', '*.perms', '*.rules', '*.pc', 'Dockerfile*'], '#'),
    filetype(None,            ['.git', '__pycache__', 'bin', 'data', 'installers', 'build', '*.arch-*', '*.user']),
    filetype(None,            ['.DS_Store', '*.xlsx', '*.pptx', '*.pdf', '*.bin', '*.ts', '*.xcf', '*.png', '*.svg', '*.ico'])
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
        elif ftype.description is not None:
            ftype.process_file(path)

# Main code.
if __name__ == '__main__':
    process_directory(tsbuild.repo_root())
    files_total = filetype('Total')
    files_cpp = filetype('C++')
    for ft in files_order:
        files_total.add(ft)
        if isinstance(ft.description, str) and ft.description.startswith(files_cpp.description):
            files_cpp.add(ft)
    filetype.description_width = max(filetype.description_width, len(files_total.description))
    filetype.description_width = max(filetype.description_width, max([len(ft.description) for ft in files_order if ft.description is not None]))
    print('## TSDuck Code Metrics')
    print()
    print('Version %s.%s-%s' % tsbuild.version())
    print()
    print('### Lines of code')
    print()
    filetype.print_lines_header()
    files_total.print_lines()
    files_cpp.print_lines()
    for ft in files_order:
        if ft.description is not None and ft.total_lines > 0:
            ft.print_lines()
    print()
    print('### Characters of code')
    print()
    filetype.print_chars_header()
    files_total.print_chars()
    files_cpp.print_chars()
    for ft in files_order:
        if ft.description is not None and ft.total_chars > 0:
            ft.print_chars()
