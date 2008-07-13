<?xml version="1.0" encoding="iso-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
	<!-- Include default bugzilla XSL -->
	<xsl:include href="bugzilla-docs.xsl"/>
	<!-- Set Chunk Specific XSL Params -->
	<xsl:param name="chunker.output.doctype-public">-//W3C//DTD HTML 4.01 Transitional//EN</xsl:param>
	<xsl:param name="chunker.output.doctype-system">http://www.w3.org/TR/html4/loose.dtd</xsl:param>
	<xsl:param name="chunk.section.depth" select="1"/>
	<xsl:param name="chunk.first.sections" select="1"/>
	<!-- Don't output filename list - mimic old behaviour-->
	<xsl:param name="chunk.quietly" select="0" />
</xsl:stylesheet>		
