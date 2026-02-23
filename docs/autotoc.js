/*! Auto-TOC script functions for Help+Manual Premium Pack
Copyright (c) 2015-2024 by Tim Green. 
All rights reserved. */

// String fix functions
function truncate(str, limit) {
var bits, i;
    if (limit == 0) return str;
    if ((limit > 0) && (limit <= 20)) limit = 20;
	bits = str.split('');
	if (bits.length > limit) {
	   for (i = bits.length - 1; i > -1; --i) {
		if (i > limit) {
		bits.length = i;
		}
		else if (' ' === bits[i]) {
		bits.length = i;
		break;
		}
	}
	bits.push('...');
	}
return bits.join('');

}
function trim(s) {
	return s.replace(/^\s+|\s+$/g, "");
}

function htmlFix(hd) {
	heading = hd.replace(/\&/g, "&amp;");
	hd = hd.replace(/</g, "&lt;");
	hd = hd.replace(/>/g, "&gt;");
	return hd;
	}

function autoTOC() {

var topicParas = new Array();
var tocHeads = new Array();
var atoc_tip = hmatocvars.atoc_tip;
var atoc_minHeaders = hmatocvars.atoc_minHeaders;
var atoc_btntip_on = hmatocvars.atoc_btntip_on;
var atoc_toptip =  hmatocvars.atoc_toptip;
var atoc_top = hmatocvars.atoc_top;
var atoc_linklimit = hmatocvars.atoc_linklimit;

var thisPara,
	linkText,
	thisParaClass,
	subStyle,
	fullLinkText,
	linkID,
	TOClink,
	autoTOCcontent,
	TOCbox,
	isATOC = false;

// Initialize ATOC tool

$("img#hmAtocLink").attr({"src": "contents.png", "title": atoc_btntip_on, "alt": atoc_btntip_on});
$("div#atocIcon span").removeClass("navOff").addClass("navOn");
$("div#atocIcon").css("display","block");
navIconInit("img#hmAtocLink");

// Tag toggle headers with icons with temporary classes for ATOC
// ":not" clause prevents doubling up when searching with the highlighter
// which re-runs the autoTOC() function

  $("span[class*='_atoc_']").parent("td:not(:has(span[class='temp_atoc_']))").each(function(){
   		var tempTogHead = $(this).html();
   		tempTogHead = '<span class="temp_atoc_">' + tempTogHead + '</span>';
   		$(this).html(tempTogHead);
   		// alert($(this).html());
   		}); 
	
    $("span[class*='_atocs_']").parent("td:not(:has(span[class='temp_atocs_']))").each(function(){
	// $("td:not(:has(span[class='temp_atocs_']))").has("span[class*='_atocs_']").each(function(){
   		var tempTogHead = $(this).html();
   		tempTogHead = '<span class="temp_atocs_">' + tempTogHead + '</span>';
   		$(this).html(tempTogHead);
 	});

	// Get the headers with AutoTOC tags
	topicParas = $("[class*='_atoc_'],[class*='_atocs_']").filter("[class^='p_']").add("span[class='temp_atoc_'],span[class='temp_atocs_']");
	
	if (topicParas.length >= atoc_minHeaders) {
	for (var i = 0; i < topicParas.length;i++) {
		thisPara = topicParas[i];
		linkText = $(topicParas[i]).text();
		linkText = trim(linkText);
		linkText = htmlFix(linkText);
		thisParaClass = $(thisPara).attr("class");
		subStyle = (thisParaClass.indexOf("_atocs_") != -1);
		// Delete non-breaking space for brain-dead IE
		if (linkText.length == 1) linkText = linkText.replace(/\xa0/,"");
		if (linkText != "") {
			isATOC = true;
			linkID = "autoTOC"+i;
			fullLinkText = linkText.replace(/\"/g,"'");
			linkText = truncate(linkText,atoc_linklimit);
			thisPara.innerHTML = '<a id="'+linkID+'"></a>' + thisPara.innerHTML;
			if (!subStyle) {
			   TOClink = '<li class="autoTOC" data="atoc" id="src_'+linkID+'" title="'+atoc_tip+fullLinkText+'"><p class="autoTOC" unselectable="on"><img src="bullet_go.png" class="menuicon" alt="'+atoc_tip+fullLinkText+'" border="0" />&nbsp;'+linkText+'</p></li>';
				} else {
			   TOClink = '<li class="autoTOC" data="atoc" id="src_'+linkID+'" title="'+atoc_tip+fullLinkText+'"><p class="autoTOC" unselectable="on" style="font-size: 90%; font-weight: normal;">&nbsp;&nbsp;<img src="bullet_go.png" class="menuicon" alt="'+atoc_tip+fullLinkText+'" border="0" />&nbsp;'+linkText+'</p></li>';
				}
			tocHeads.push(TOClink);
				}
	}

		} else return;

	// Build the AutoTOC if elements exist

	if ((tocHeads[0]) && (tocHeads[0] != "")) {
	autoTOCcontent = "";
	TOCbox = document.getElementById("autoTocWrapper");
	for (var i = 0; i < tocHeads.length;i++) {
		autoTOCcontent = autoTOCcontent + tocHeads[i];
		}
		
	autoTOCcontent = '<li id="toplink" title="'+atoc_toptip+'"><p class="autoTOC" unselectable="on" data="atoc"><img src="application_get.png" class="menuicon" alt="'+atoc_toptip+'" border="0" />&nbsp;'+atoc_top+'</p></li>' +autoTOCcontent;
autoTOCcontent = '<div id="autoTocMiddle" class="tmenumiddle"><div id="autoTocInner" class="tmenuinner"><ul>' + autoTOCcontent + '</ul></div></div>';

	TOCbox.innerHTML = autoTOCcontent;
	} // End Build ATOC

	// Function for getting tagnames
	$.fn.tagName = function() {
   	return this.get(0).tagName;
	}
		
	// Initialize 
	initMenu("atocIcon","autoTocWrapper");	
	
    // Jump to targets for main ATOC entries
     $("li.autoTOC[data='atoc']").click(function () {
     	
		var isSearch = SearchCheck();
     	var cTarget = $(this).attr("id");
    	var tTarget = cTarget.replace(/src_/,"");
        var theTarget = $("a[id='"+tTarget+"']");

	   // If there are toggles on the page close them all
	   if ((HMToggles.length != null) && (!isSearch)) {
	   HMToggleExpandAll(false); 
	   }

	   // Scroll to the target
	   aniScroll(theTarget,"menu",false,false);
   
		return false;
    });

    // Scroll to top of document function
	$("#toplink").click(function () {
    var isSearch = SearchCheck();
	if (HMToggles.length != null && !isSearch) HMToggleExpandAll(false);
	var sTarget = "div#idcontent";
	$(sTarget).scrollTo(0, 300);
	return false;
	});

} // End autoTOC() function
// Flag variable for checking load
var atocLoaded = true;
