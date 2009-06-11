<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet
   version="1.0"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
   xmlns="http://www.w3.org/1999/xhtml">
  <xsl:output method="xml" indent="yes" encoding="UTF-8"/>

  <!-- Emit main page -->
  <xsl:template match="/moonunit">
    <html>
      <head>
	<xsl:choose>
	  <xsl:when test="@name">
	    <title>MoonUnit: <xsl:value-of select="@name"/></title>
	  </xsl:when>
	  <xsl:otherwise>
	    <title>MoonUnit Test Results</title>
	  </xsl:otherwise>
	</xsl:choose>
	<link rel="stylesheet" type="text/css" href="muxhtml.css"/>
	<script type="text/javascript" src="muxhtml.js">
	  <xsl:comment>This space intentionally left blank</xsl:comment>
	</script>
      </head>
      <body onload="collapse_tests()">
	<div class="main">
	  <div class="header">
	    <xsl:if test="@name">
	      <span class="title">
		<xsl:value-of select="@name"/>
	      </span>
	    </xsl:if>
	  </div>
	  <div class="content">
	    <xsl:call-template name="emit-summary"/>
	    <xsl:for-each select="platform">
	      <xsl:call-template name="emit-platform"/>
	    </xsl:for-each>
	  </div>
	</div>
      </body>
    </html>
  </xsl:template>

  <xsl:template name="emit-summary">
    <div class="summary">
      <div class="header">
	<span class="title">Summary</span>
      </div>
      <div class="content">
	<div class="statistic" name="tests">
	  <div>Tests</div>
	  <div><xsl:value-of select="count(.//test)"/></div>
	</div>
	<div class="statistic" name="passed">
	  <div>Passed</div>
	  <div>
	    <xsl:value-of 
	       select="count(.//test[result/@status = 'pass' or result/@status = 'xfail'])"
	       />
	  </div>	 
	</div>
	<div class="statistic" name="failed">
	  <div>Failed</div>
	  <div>
	    <xsl:value-of 
	       select="count(.//test[result/@status = 'fail' or result/@status = 'xpass'])"
	       />
	  </div>
	</div>
	<div class="statistic" name="skipped">
	  <div>Skipped</div>
	  <div>
	    <xsl:value-of 
	       select="count(.//test[result/@status = 'skip'])"
	       />
	  </div>
	</div>
      </div>
    </div>
  </xsl:template>

  <!-- Emit platform entry -->
  <xsl:template name="emit-platform">
    <div class="platform">
      <div class="header">
	<span class="title">
	  <span name="cpu"><xsl:value-of select="@cpu"/></span>
	  <span name="vendor"><xsl:value-of select="@vendor"/></span>
	  <span name="os"><xsl:value-of select="@os"/></span>
	</span>
      </div>
      <div class="content">
	<xsl:for-each select="library">
	  <xsl:call-template name="emit-library"/>
	</xsl:for-each>
      </div>
    </div>
  </xsl:template>

  <!-- Emit library entry -->
  <xsl:template name="emit-library">
    <div class="library">
      <div class="header">
	<div class="title"><xsl:value-of select="@name"/></div>
      </div>
      <div class="content">
	<xsl:for-each select="suite">
	  <xsl:call-template name="emit-suite"/>
	</xsl:for-each>
      </div>
    </div>
  </xsl:template>

  <!-- Emit suite entry -->
  <xsl:template name="emit-suite">
    <div class="suite">
      <div class="header">
	<span class="title"><xsl:value-of select="@name"/></span>
      </div>
      <div class="content">
	<xsl:for-each select="test">
	  <xsl:call-template name="emit-test"/>
	</xsl:for-each>
      </div>
    </div>
  </xsl:template>

  <!-- Emit test entry -->
  <xsl:template name="emit-test">
    <xsl:variable name="status" select="result/@status"/>
    <xsl:variable name="cid" select="generate-id()"/>
    <div class="test">
      <div class="header" onclick="activate_content('{$cid}')">
	<xsl:if test="event or normalize-space(result/.)">
	  <xsl:attribute name="full"><xsl:text>true</xsl:text></xsl:attribute>
	</xsl:if>
	<span class="result">
	  <xsl:attribute name="status">
	    <xsl:choose>
	      <xsl:when test="$status = 'pass' or $status = 'xfail'">
		<xsl:text>pass</xsl:text>
	      </xsl:when>
	      <xsl:when test="$status = 'fail' or $status = 'xpass'">
		<xsl:text>fail</xsl:text>
	      </xsl:when>
	      <xsl:when test="$status = 'skip'">
		<xsl:text>skip</xsl:text>
	      </xsl:when>
	    </xsl:choose>
	  </xsl:attribute>
	  <xsl:value-of select="$status"/>
	</span>
	<span class="title"><xsl:value-of select="@name"/></span>
      </div>
      <div class="content" id="{$cid}" name="test-content">
	<xsl:for-each select="event">
	  <xsl:call-template name="emit-event"/>
	</xsl:for-each>
	<!-- Emit file/line number if we know it -->
	<xsl:if test="result/@file">
	  <span class="source">
	    <xsl:value-of select="result/@file"/>
	    <xsl:text>:</xsl:text>
	    <xsl:value-of select="result/@line"/>
	  </span>
	</xsl:if>
	<span class="message">
	  <xsl:value-of select="normalize-space(result)"/>
	</span>
      </div>
    </div>
  </xsl:template>
  
  <!-- Emit event entry -->
  <xsl:template name="emit-event">
    <div class="event">
      <xsl:attribute name="level">
	<xsl:value-of select="@level"/>
      </xsl:attribute>
      <xsl:if test="@file">
	<span class="source">
	  <xsl:value-of select="@file"/>
	  <xsl:text>:</xsl:text>
	  <xsl:value-of select="@line"/>
	</span>
      </xsl:if>
      <span class="level"><xsl:value-of select="@level"/></span>
      <span class="message">
	<xsl:value-of select="normalize-space(.)"/>
      </span>
    </div>
  </xsl:template>
</xsl:stylesheet>
