<xsl:transform version="1.0"
	       xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	       xmlns:fn="http://www.w3.org/2005/02/xpath-functions"
           xmlns:html="http://www.w3.org/1999/xhtml">
<xsl:output method="xml" doctype-public="-//W3C//DTD XHTML 1.1//EN" doctype-system="http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd" indent="yes" />

  <xsl:template match="/">
    <html xmlns="http://www.w3.org/1999/xhtml">
      <head>
	    <title>Moonunit Test Results</title>
	    <link rel="stylesheet" type="text/css" href="moonunit-style.css" />
      </head>
      <body>
	    <h1>Moonunit Test Results</h1>
	    <xsl:apply-templates/>
      </body>
    </html>
  </xsl:template>
  <xsl:template match="library">
    <xsl:variable name="total" select="count(suite/test)" />
    <xsl:variable name="passed" select="count(suite/test/result[@status='pass'])" />
    <xsl:variable name="failed" select="count(suite/test/result[@status='fail'])" />
    <table xmlns="http://www.w3.org/1999/xhtml" class="test-table">
      <tr class="test-header-library">
	    <th colspan="2">Library <xsl:value-of select="@name" /></th>
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
	  <xsl:value-of select="$total" /> total,
	  <xsl:value-of select="$passed" /> passed,
	  <xsl:value-of select="$failed" /> failed
	</td>
      </tr>
    </table>
  </xsl:template>
  <xsl:template match="suite">
    <tr xmlns="http://www.w3.org/1999/xhtml" class="test-row-suite">
      <td colspan="2"><xsl:value-of select="@name" /></td>
    </tr>
    <xsl:apply-templates/>
  </xsl:template>
  <xsl:template match="test">
    <xsl:variable name="result" select="result/@status" />
    <xsl:variable name="contents" select="result/." />
    <tr xmlns="http://www.w3.org/1999/xhtml" class="test-row-test">
      <td class="test-name"><xsl:value-of select="@name" /></td>
      <xsl:choose>
	<xsl:when test="$result = 'pass'">
	  <td class="test-result"><span class="text-pass">pass</span></td>
	</xsl:when>
	<xsl:when test="$result = 'fail'">
	  <td class="test-result"><xsl:value-of select="result/@stage" /> <span class="text-fail">fail</span></td>
	</xsl:when>
      </xsl:choose>
    </tr>
    <xsl:if test="string-length($contents) > 0">
      <tr xmlns="http://www.w3.org/1999/xhtml" class="test-row-reason">
	    <td class="test-source">
          <xsl:if test="result/@line">
            <xsl:value-of select="result/@file"/>:<xsl:value-of select="result/@line"/>
          </xsl:if>
        </td>
	    <td class="last"><xsl:value-of select="." /></td>
      </tr>
    </xsl:if>
    <xsl:apply-templates/>
    <xsl:if test="following-sibling::*">
      <tr xmlns="http://www.w3.org/1999/xhtml" class="test-row-divider">
	<td colspan="2" />
      </tr>
    </xsl:if>
  </xsl:template>
  <xsl:template match="event">
    <xsl:variable name="status" select="@level"/>
    <tr class="test-row-event" xmlns="http://www.w3.org/1999/xhtml">
      <td class="first"/>
      <xsl:choose>
        <xsl:when test="$status = 'warning'">  
          <td class="test-result"><xsl:value-of select="@stage"/><span class="text-warning"><xsl:value-of select="@level"/></span></td>
        </xsl:when>
        <xsl:when test="$status = 'info'">  
          <td class="test-result"><xsl:value-of select="@stage"/><span class="text-info"><xsl:value-of select="@level"/></span></td>
        </xsl:when>
        <xsl:when test="$status = 'verbose'">  
          <td class="test-result"><xsl:value-of select="@stage"/><span class="text-verbose"><xsl:value-of select="@level"/></span></td>
        </xsl:when>
        <xsl:when test="$status = 'trace'">  
         <td class="test-result"><xsl:value-of select="@stage"/><span class="text-trace"><xsl:value-of select="@level"/></span></td>
        </xsl:when>
      </xsl:choose>
    </tr>
    <tr class="test-row-reason" xmlns="http://www.w3.org/1999/xhtml">
      <td class="test-source">
        <xsl:if test="@line">
          <xsl:value-of select="@file"/>:<xsl:value-of select="@line"/>
        </xsl:if>
      </td>
      <td class="last"><xsl:value-of select="."/></td>
    </tr>
  </xsl:template>
  <xsl:template match="test/result" />
</xsl:transform>
