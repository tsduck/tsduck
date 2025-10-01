#-----------------------------------------------------------------------------
#
# TSDuck - The MPEG Transport Stream Toolkit
# Copyright (c) 2005-2025, Thierry Lelegard
# BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
#
# Macros for asciidoctor documents.
# Warning: being completely ignorant in Ruby, the code here was adapted from
# various examples online. It may look awful to Ruby developers.
#
#-----------------------------------------------------------------------------

require 'asciidoctor/extensions'
include ::Asciidoctor

# Macro to create a link to a TSDuck C++ class in the programming reference.
# Top-level example: class "ts::FooBar" -> classref:FooBar[]
# Nested example: class "ts::foo::Bar" -> classref:foo/Bar[]
class ClassRefMacro < Asciidoctor::Extensions::InlineMacroProcessor
  use_dsl
  named :classref

  # Process the macro. Target is the classname. There is no attribute.
  def process parent, target, attrs
    doc = parent.document
    # TSDuck home page is extracted from '{home}' attribute in document.
    home = (doc.attr? 'home') ? (doc.attr 'home') : 'http://'
    # Target URL.
    url = %(#{home}doxy/class/#{target.gsub('/','__')}.html)
    # Create the asciidoctor content to be reprocessed.
    create_inline_pass parent, %(`#{url}[ts::#{target.gsub('/','::')}]`), attributes: { 'subs' => :normal }
  end
end

# Macro to create a link to a doxygen group in the programming reference.
# Example for group "python": groupref:python[Python binding]
class GroupRefMacro < Asciidoctor::Extensions::InlineMacroProcessor
  use_dsl
  named :groupref
  name_positional_attributes 'text'

  # Process the macro. Target is the groupname. See comments in ClassRefMacro.
  def process parent, target, attrs
    doc = parent.document
    text = attrs['text']
    home = (doc.attr? 'home') ? (doc.attr 'home') : 'http:'
    url = %(#{home}doxy/group/#{target}.html)
    create_inline_pass parent, %(#{url}[#{text}]), attributes: { 'subs' => :normal }
  end
end

# Register the inline macros to asciidoctor.
Asciidoctor::Extensions.register do
  inline_macro ClassRefMacro
  inline_macro GroupRefMacro
end
