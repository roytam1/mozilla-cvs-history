<?xml version="1.0" encoding="iso-8859-1"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format" version="1.0">
    <!-- Enable passivetex extensions -->
	<xsl:param name="passivetex.extensions" select="1"/>
    <xsl:param name="tablecolumns.extensions" select="1"/>
    
	<!-- Show <ulink>s as footnotes -->
    <xsl:param name="ulink.footnotes" select="1" />
	<xsl:param name="ulink.show" select="1" />
	
	<!-- Don't use Graphics -->
	<xsl:param name="admon.graphics" select="0"/>
	<xsl:param name="callout.graphics" select="'0'"/>
	
	<xsl:template match="simplelist[@type='inline']/member">
        <xsl:apply-templates/>
    </xsl:template>

</xsl:stylesheet>
