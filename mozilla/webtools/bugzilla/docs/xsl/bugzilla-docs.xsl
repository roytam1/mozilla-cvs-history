<?xml version="1.0" encoding="iso-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- Nicer Filenames -->
<xsl:param name="use.id.as.filename" select="1"/>

<!-- Label sections if they aren't automatically labeled -->
<xsl:param name="section.autolabel" select="1"/>

<!-- Table of Contents Depth -->
<xsl:param name="toc.section.depth">1</xsl:param>

<!-- Set chunk parameters -->
<xsl:param name="chunk.section.depth" select="1"/>
<xsl:param name="chunk.first.sections" select="1"/>
<xsl:param name="chunker.output.encoding" select="UTF-8"/>

<!-- Show titles of next/previous page -->
<xsl:param name="navig.showtitles">1</xsl:param>

<!-- Tidy up the HTML a bit... -->
<xsl:param name="html.cleanup" select="1"/>
<xsl:param name="make.valid.html" select="1"/>
<xsl:param name="html.stylesheet">api/style.css</xsl:param>
<!-- make links nicer... -->
<xsl:param name="refentry.generate.title" select="1"/>
<xsl:param name="refentry.generate.name" select="0"/>

<!-- Use Graphics, specify their Path and Extension -->
<xsl:param name="admon.graphics" select="1"/>
<xsl:param name="admon.graphics.path">../images/</xsl:param>
<xsl:param name="admon.graphics.extension">.gif</xsl:param>

<xsl:param name="qanda.inherit.numeration" select="0" />

<!--
****
CODE BELOW HERE IS EXTRACTED AND EDITED FROM THE DOCBOOK XSL SOURCES
****
-->

<xsl:template match="simplelist[@type='inline']/member">
    <xsl:apply-templates/>
</xsl:template>

<!--
To generate valid HTML, we need to redefine this section... Code extracted from
http://cvs.sourceforge.net/viewcvs.py/docbook/xsl/html/qandaset.xsl?rev=1.19&view=log

and modified below. Basic change: Remove the colspan attribute of the tr tags - no
other changes have been made to the document.
-->

<xsl:template match="qandadiv">
  <xsl:variable name="preamble" select="*[name(.) != 'title'
                                          and name(.) != 'titleabbrev'
                                          and name(.) != 'qandadiv'
                                          and name(.) != 'qandaentry']"/>

  <xsl:if test="blockinfo/title|title">
    <tr class="qandadiv">
      <td align="left" valign="top" colspan="2">
        <xsl:call-template name="anchor">
          <xsl:with-param name="conditional" select="0"/>
        </xsl:call-template>
        <xsl:apply-templates select="(blockinfo/title|title)[1]"/>
      </td>
    </tr>
  </xsl:if>

  <xsl:variable name="toc">
    <xsl:call-template name="dbhtml-attribute">
      <xsl:with-param name="pis"
                      select="processing-instruction('dbhtml')"/>
      <xsl:with-param name="attribute" select="'toc'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="toc.params">
    <xsl:call-template name="find.path.params">
      <xsl:with-param name="table" select="normalize-space($generate.toc)"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="(contains($toc.params, 'toc') and $toc != '0') or $toc = '1'">
    <tr class="toc">
      <td align="left" valign="top" colspan="2">
        <xsl:call-template name="process.qanda.toc"/>
      </td>
    </tr>
  </xsl:if>
  <xsl:if test="$preamble">
    <tr class="toc"	>
      <td align="left" valign="top" colspan="2">
        <xsl:apply-templates select="$preamble"/>
      </td>
    </tr>
  </xsl:if>
  <xsl:apply-templates select="qandadiv|qandaentry"/>
</xsl:template>
</xsl:stylesheet>

