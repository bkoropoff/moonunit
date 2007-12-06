<xsl:transform version="1.0"
	       xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	       xmlns:fn="http://www.w3.org/2005/02/xpath-functions">
  <xsl:template match="/">
    <html>
      <head>
	<title>Moonunit Test Results</title>
	<link rel="stylesheet" type="text/css" href="test.css"/>
      </head>
      <body>
	<h1>Moonunit Test Results</h1>
	<xsl:apply-templates/>
      </body>
    </html>
  </xsl:template>
  <xsl:template match="library">
    <xsl:variable name="total" select="count(suite/test)"/>
    <xsl:variable name="passed" select="count(suite/test[@result='pass'])"/>
    <xsl:variable name="failed" select="count(suite/test[@result='fail'])"/>
    <table class="test-table">
      <tr class="test-header-library">
	<th colspan="2">Library <xsl:value-of select="@name"/></th>
      </tr>
<!--
      <tr class="test-header-columns">
	<th colspan="2">Test</th>
	<th>Result</th>
      </tr>
-->
      <xsl:apply-templates/>
      <tr class="test-row-summary">
	<td class="first">Summary</td>
	<td class="last">
	  <xsl:value-of select="$total"/> total,
	  <xsl:value-of select="$passed"/> passed,
	  <xsl:value-of select="$failed"/> failed
	</td>
      </tr>
    </table>
  </xsl:template>
  <xsl:template match="suite">
    <tr class="test-row-suite">
      <td colspan="2"><xsl:value-of select="@name"/></td>
    </tr>
    <xsl:apply-templates/>
  </xsl:template>
  <xsl:template match="test">
    <xsl:variable name="result" select="@result"/>
    <xsl:variable name="contents" select="."/>
    <tr class="test-row-test">
      <td class="test-name"><xsl:value-of select="@name"/></td>
      <xsl:choose>
	<xsl:when test="$result = 'pass'">
	  <td class="test-result"><span class="text-pass">pass</span></td>
	</xsl:when>
	<xsl:when test="$result = 'fail'">
	  <td class="test-result"><xsl:value-of select="@stage"/> <span class="text-fail">fail</span></td>
	</xsl:when>
      </xsl:choose>
    </tr>
    <xsl:if test="string-length($contents) > 0">
      <tr class="test-row-reason">
	<td class="first"/>
	<td class="last"><xsl:value-of select="."/></td>
      </tr>
    </xsl:if>
    <xsl:if test="following-sibling::*">
      <tr class="test-row-divider">
	<td colspan="2"/>
      </tr>
    </xsl:if>
  </xsl:template>
</xsl:transform>
