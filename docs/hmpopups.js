/*! Help+Manual Advanced JavaScript Popups 
For Help+Manual Premium Pack CHM
Copyright (c) 2015-2024 by Tim Green
All Rights Reserved */

// Config variables objects

var popCfg = {};
var hpop = {};
var hmParams = "";


// Listener for mouse position
var scrBar = false;
function hmpopScrListen(e) {
	// Prevent cursor icon bug in WebKit browsers by deactivating default move behavior
	// Also prevents unwanted image dragging in popups for some browsers

	e.preventDefault();
	 var bdWidth = parseInt($("#hmpopupdiv").css("borderWidth"),10);
	 var offset = $("#hmpopupdiv").offset();
	 var sWidth = $("#hmpopupdiv").outerWidth()-(20 + bdWidth);
	 var sHeight = $("#hmpopupdiv").outerHeight()-(20 + bdWidth);
	 var mX = e.pageX-offset.left;
	 var mY = e.pageY-offset.top;

	if (mX > sWidth || mY > sHeight){
		scrBar = true;
		$("#hmpopupdiv").css("cursor","auto");
		}
	else {
		scrBar = false;
		$("#hmpopupdiv").css("cursor","move");
		}
}

// RGB to Hex converter
function rgbHex(color) {
	if (color.substr(0, 1) === '#') {
		return color;
	}
	var digits = /(.*?)rgb\((\d+), (\d+), (\d+)\)/.exec(color);
	var red = parseInt(digits[2]);
	var green = parseInt(digits[3]);
	var blue = parseInt(digits[4]);
	var rgb = blue | (green << 8) | (red << 16);
	return digits[1] + '#' + rgb.toString(16);
}

// 	Error trapper for CHM
function traperror(){return true;}

// Initialize 
$(document).ready(function() {
	
// Execute if there are popups on the page 
if ($("a.popuplink").length > 0 || $("a[onclick^='return hmshowPopup']").length > 0 || $("input[onclick^='return hmshowPopup']").length > 0 || $("area[href^='javascript:void(0)']").length > 0) {
	$("body").append('<div id="hmpopupdiv" />');
	$("div#hmpopupdiv").css({visibility: "visible", display: "none", width: "auto", overflow: "auto", "z-index": "10000"});
	// if (!hpop.isvideo) tooltip.style.position = hpop.ie9 ? "fixed" : hpop.header ? "relative" : "absolute"; 
 	// Global variables and browser detection
	hpop.ua = navigator.userAgent.toLowerCase();
	hpop.dom = (document.getElementById) ? true : false;
	hpop.ie4 = (document.all && !hpop.dom) ? true : false;
    hpop.ie5_5 = ((hpop.ua.indexOf("msie 5.5") >= 0) && (hpop.ua.indexOf("opera") < 0)) ? true : false;
	hpop.ie6 = ((hpop.ua.indexOf("msie 6") >= 0) && (hpop.ua.indexOf("opera") < 0)) ? true : false;
	hpop.ns4 = (document.layers && !hpop.dom) ? true : false;
	var ie9rx = /trident\/([\d])/m;
	var match = ie9rx.exec(hpop.ua);
	if (match != null) {
		hpop.ie9 = !!(parseInt(match[1],10) >= 5);
	} else {
		hpop.ie9 = false;
	}
	hpop.ie8 = (hpop.ua.indexOf("trident/4") >= 0);
	hpop.is_ie = ((hpop.ua.indexOf("msie") >= 0) && (hpop.ua.indexOf("opera") < 0));
	hpop.is_webkit = hpop.ua.indexOf("webkit") > 0;
	hpop.iequirks = (hpop.is_ie) && (document.compatMode.toLowerCase() == "backcompat");
	hpop.is_ff = ((hpop.ua.indexOf("firefox") >=0) && (!hpop.is_ie) && (hpop.ua.indexOf("opera") < 0));
	hpop.is_chm = (location.href.search("::")>=0);
	hpop.is_ns = document.getElementById("innerdiv") ? true : false;
	hpop.offsxy = 6;
	hpop.isvideo = false;
	hpop.smallPopup = false;
	hpop.bdDrag = false;
	hpop.reSize = "nw-resize";
	hpop.ewResize = (!hpop.is_ie && !hpop.is_chm) ? "ew-resize" : "e-resize" ;
	hpop.nsResize = (!hpop.is_ie && !hpop.is_chm) ? "ns-resize" : "s-resize" ;
	hpop.popclick = false;
	
	// Position mode
	if (hpop.ie9) $("div#hmpopupdiv").css("position","fixed");
		else $("div#hmpopupdiv").css("position","absolute");
	
	// Activate if modern browser running, otherwise leave standard untouched.
	if (!hpop.ie4 && !hpop.ie5_5 && !hpop.ns4) {
	
	// Remap the standard popup function
	hmshowPopup = function(a, b, c) {
	hmNewPopup(a, b, c);
	}
	
	// Define the configuration variables
	popCfg.bWidth = parseInt($("div#hmpopupdiv").css("border-left-width"),10); // Popup border width
	popCfg.pPadding = parseInt($("div#hmpopupdiv").css("paddingTop"),10); // Popup padding
	popCfg.pbgColor = rgbHex($("div#hmpopupdiv").css("background-color")); // = popCfg.pbgColor ?  "#" + popCfg.pbgColor : "#fdfcdc";
	popCfg.bdColor = rgbHex($("div#hmpopupdiv").css("border-left-color")).replace("#",""); // popCfg.bdColor ? popCfg.bdColor : "112233"; // 
	popCfg.smallDim = window.hmPopSmallDim;
	popCfg.killVideo = window.hmPopKillVideo;

	// Trap spurious timing errors in CHM
	if (hpop.is_chm) {
		window.onerror=traperror;
		}
	} // End activation block
}
		
});

// General wrapper function
function hmNewPopup(e,txt,stick){
  
// Kill popup on click outside popup and not in scrollbar
var hmClickKill = function(e) {
	var isPopup=false;
	var cP = (!hpop.is_ie) ? e.target : event.srcElement;
	var vpW = $(window).width(), vpH = $(window).height();

	if (!hpop.is_ie || (hpop.is_ie && hpop.is_ns)) {
		vpW = vpW - 20; vpH = vpH - 20;
		}
	var tName = cP.id;
		do {
			tName=cP.id;
			if (tName && tName=="hmpopupdiv") {
				isPopup=true;
				}
			cP=cP.parentNode;
		} while (tName!=="hmpopupdiv" && cP.parentNode);
		if (isPopup) {
			return false;
			}
		else
		{
			if ((e.clientX > vpW) || (e.clientY > vpH) || hpop.bdDrag){
					return false;
			}
		hmKillPopup(true);
		return false;
		}
}; // End hmClickKill


	// Kill any visible popup and remove video background to reset
	$("div#hmpopupdiv, #vidbg").stop(false,true);
	hmKillPopup(false);
	$("#vidbg").remove();
	// Popup component variables declarations
	var tooltip = {};

	// Drag and resize variables
	var doDrag=false;
	var doPopResize=false;
	var popClosed=false;
	var is_absolute=false;
	var popX=0;
	var	popY=0;
	var tLeft=0;
	var tTop=0;
	var currHeight=0;
	var currWidth=0;
	var popW = 0;
	var popH = 0;
	
	// Configuration variables
	var bWidth = popCfg.bWidth; // Popup border width
	var pPadding = popCfg.pPadding;  // Popup padding
	var pbgColor = popCfg.pbgColor; // Popup background color;
	var bdColor = popCfg.bdColor; // Popup border Color

	var bdColorR; // Components for RGB
		var bdColorG;
		var bdColorB;
	var bdTop; // Individual border colors, will be calculated later
	var bdRight;
	var bdBottom;
	var bdLeft;
	var pShadow = window.hmpBShadow; // Boolean for shadow effect for border
	var smallDim = popCfg.smallDim ? popCfg.smallDim : 500;
	if (smallDim<500) smallDim = 500;
 
	// Hex<>Dec converters
	function d2h(d) {return d.toString(16);}
	function h2d(h) {return parseInt(h,16);}
	// Return two digit hex values only
	function h2two(h) {return (h.length == 1) ? "0" + h : h;}
	// Stop adjustments exceeding 240 dec or lower than 0
	function hstop(h) {return h > 240 ? 240 : h < 0 ? 0 : h ;}
    // Apply the adjustments
	function bdFix(x) {
		var R,G,B;
		R = h2two(d2h(hstop(x + bdColorR)));
		G = h2two(d2h(hstop(x + bdColorG)));
		B = h2two(d2h(hstop(x + bdColorB)));
		return "#" + R + G + B;
		}

	// Popup border colors, faux shadow effect
	if (!pShadow) {
		bdBottom = bdTop = bdLeft = bdRight = "#" + d2h(bdColor);
		} else {
		bdColorR = h2d(bdColor.substring(0,2));
		bdColorG = h2d(bdColor.substring(2,4));
		bdColorB = h2d(bdColor.substring(4,6));
		bdBottom = bdFix(0);
		bdRight = bdFix(20);
		bdTop = bdFix(180);
		bdLeft = bdFix(180);
		}

// Check for embedded video, popup is not resizable or draggable with video
	hpop.isvideo = /(?:<video)|(?:&lt;video)|(?:<object[\s\S]*?(\.swf|\.flv|\.avi|\.mpg|\.mov|\.ram|\.mp4|\.mkv|\.m4v|\.wmv|\.divx|\.dvr-ms|\.f4v|\.fli|\.m2p|\.mpeg|\.mpeg4|\.ogm|\.ogv|\.ogx|\.xvid)[\s\S]*?<\/object>)/im.test(txt);

// IE6 needs a table wrapper to automatically size popups that don't have their own table properly
	if (hpop.ie6) txt = '<table border="0" cellpadding="0" cellspacing="0"><tr><td valign="top">' + txt + '</td></tr></table>';

	// Fill the div and rewrite the links
	var tempPop = $("div#hmpopupdiv");
	$(tempPop).html(txt);
	// hmPopRewriteLinks("#hmpopupdiv ");

	// Everything is ready, let's roll...
	setTimeout(function(){
		hmDoPopup(e);
	},0);

function dragPopup(e){
	
	// How far has the mouse moved?
	var moveX = (!hpop.is_ie) ? e.clientX - popX : event.clientX - popX;
	var moveY = (!hpop.is_ie) ? e.clientY - popY : event.clientY - popY;
		if (doDrag) {

			tooltip.style.left = tLeft + moveX + "px";
			tooltip.style.top = tTop + moveY + "px";
			if (hpop.is_ie) {
				tooltip.style.zoom = "1";
				tooltip.style.filter = "alpha(opacity=40)";
			}
			else {
			tooltip.style.opacity = '0.4';
			}
			// Prevent selection during dragging
			deSelect();
			return false;
		}
		// }
	} // End dragPopup()

function resizePopup(e){

	// How far has the mouse moved?
	var moveX = (!hpop.is_ie) ? e.clientX - popX : event.clientX - popX;
	var moveY = (!hpop.is_ie) ? e.clientY - popY : event.clientY - popY;

	if (doPopResize) {

		// Resize no further than minimum dimensions 

		if (popW + moveX > 130 && (hpop.reSize == "nw-resize" || hpop.reSize == hpop.ewResize)) {
		tooltip.style.width = (popW + moveX) + "px";
		}
		if (popH + moveY > 70 && (hpop.reSize == "nw-resize" || hpop.reSize == hpop.nsResize)) {
		tooltip.style.height = (popH + moveY) + "px";
		}
		// Keep cursor set so that it doesn't change during resizing
		$("body").css("cursor",hpop.reSize);
		// Turn off scrollbars during resizing to prevent contents shifting
		tooltip.style.overflow = "hidden";
		// Prevent selection during resizing
		deSelect();
		// Keep resize handle visible and maintain cursor mode for brain-dead IE
		return false;
	}
} // End resizePopup()

// Listener for drag and resize events
function dragger(e){
	var dPop = (!hpop.is_ie) ? e.target : event.srcElement;

	var tName = dPop.id;
	if (tName!="hmpopupdiv") {
		for (var cP = dPop;((cP.id=="hmpopupdiv" || !cP.id) && cP.parentNode); cP = cP.parentNode)
		{
			tName=cP.id;
			if (tName=="hmpopupdiv") {
				break;
				}
		}
	}
	if (hpop.bdDrag && !hpop.isvideo) {
		toggleSelect(document.body,true);
		doPopResize = true;
		hpop.reSize = $("body").css("cursor");
		popX = (!hpop.is_ie) ? e.clientX : event.clientX;
		popY = (!hpop.is_ie) ? e.clientY : event.clientY;
		popW = parseInt(tooltip.style.width,10);
		popH = parseInt(tooltip.style.height,10);
		addEventX(document,"mousemove", resizePopup);
		return false;
	}

	if (tName == 'hmpopupdiv' && !scrBar && !hpop.isvideo) {
		toggleSelect(document.body,true);
		doDrag = true;
		tLeft = parseInt(tooltip.style.left,10);
		tTop = parseInt(tooltip.style.top,10);
		popX = (!hpop.is_ie) ? e.clientX : event.clientX;
		popY = (!hpop.is_ie) ? e.clientY : event.clientY;
		
		// Only activate drag if the mouse is not on scrollbars in popup
		if ((((popX - $(tooltip).position().left) < ($(tooltip).width() - 14) ) || (tooltip.clientHeight >= tooltip.scrollHeight)) &&  
	((popY - $(tooltip).position().top) < ($(tooltip).height()-14) || (tooltip.clientWidth >= tooltip.scrollWidth))) {		
		addEventX(document, "mousemove", dragPopup);
		}
		return false;
	}
} // End dragger() drag/resize listener

// Reset after a drag or resize
function unDrag(){
	if (doDrag) {
		doDrag = false;
		scrBar = false;
	if (hpop.is_ie){
		tooltip.style.removeAttribute('filter');
		tooltip.style.removeAttribute('zoom');
	} else  {
	tooltip.style.opacity = '1.0';
		}
	removeEventX(document, "mousemove", dragPopup);
	toggleSelect(document.body, false);
	}
	else if (doPopResize) {
		doPopResize = false;
		removeEventX(document, "mousemove", resizePopup);
		toggleSelect(document.body, false);
		tooltip.style.overflow = "auto";
	}
	return false;
}  // End unDrag()

// Event handlers
function addEventX(obj, type, fn){
	if (obj.attachEvent) {
		obj['e' + type + fn] = fn;
		obj[type + fn] = function(){
			obj['e' + type + fn](window.event);
		};
		obj.attachEvent('on' + type, obj[type + fn]);
	}
	else {
		obj.addEventListener(type, fn, false);
		}
}
function removeEventX(obj, type, fn){
	if (obj.detachEvent) {
		obj.detachEvent('on' + type, obj[type + fn]);
		obj[type + fn] = null;
		obj['e' + type + fn] = null;
	}
	else {
		obj.removeEventListener(type, fn, false);
		}
} // End event handlers

// Deselect during drag
function deSelect(){
	if (window.getSelection){
		window.getSelection().removeAllRanges();
	}
	else if (document.selection){
		document.selection.empty();
	}
return false;
} // End deSelect()

// Disable selection during drag to prevent selection flicker
function toggleSelect(elem, toggler){

	function doFalse(){
		return false;
	}
	if (toggler) {
	if (hpop.is_ie) {
		addEventX(elem,"selectstart",doFalse);
		}
	else if (typeof elem.style.MozUserSelect!="undefined")
	{
		elem.style.MozUserSelect="none";
	}

	else {
		addEventX(elem,"mousedown",doFalse);
	}

	}
	else {
	if (hpop.is_ie) {
		removeEventX(elem,"selectstart",doFalse);
		}
	else if (typeof elem.style.MozUserSelect!="undefined")
		{
			elem.style.MozUserSelect="elements";
		}

	else {
		removeEventX(elem,"mousedown",doFalse);
		}
	}
} // End toggleSelect()

/* Main Popup function */

function hmDoPopup(e){
 	popClosed = false;
	// Assign tooltip variable
	tooltip = document.getElementById('hmpopupdiv');

	e = e ? e : window.event;

	var mx = e.clientX;
	var my = e.clientY;
	var bodyl = $("body").scrollLeft();
	var bodyt = $("body").scrollTop();
	var bodyw = $(window).width();
	var bodyh = $(window).height();
	var tipw = $(tooltip).outerWidth();
	var tiph = $(tooltip).outerHeight();
	var hoffset = hpop.ie9 ? pPadding : pPadding * 2;

	// Set user-defined style choices
	// tooltip.style.borderWidth = bWidth + "px";
	tooltip.style.padding = pPadding + "px";

	// Border and background colors
	tooltip.style.borderColor = bdTop + " " + bdRight + " " + bdBottom + " " + bdLeft;
	// tooltip.style.backgroundColor = pbgColor;

	// Initial tooltip position in top left corner
	tooltip.style.left = "0px";
	tooltip.style.top = "0px";
 	// Make popup fixed dimensions
	if (!hpop.ie9) {
	$(tooltip).css({"height": ($(tooltip).height()-hoffset) + "px", "width": $(tooltip).width() + "px"});
		} else {
		$(tooltip).css({"height": ($(tooltip).outerHeight()-hoffset) + "px", "width": $(tooltip).outerWidth() + "px"});
		}

	tiph = $(tooltip).outerHeight();
	tipw = $(tooltip).outerWidth();

	// Adjust dimensions if popup doesn't fit comfortably in the viewport
	if (!hpop.isvideo) {
		if (tipw > (Math.round(bodyw * 0.9))) {
			tooltip.style.width = hpop.iequirks ? Math.round(bodyw * 0.9) + "px" : ((Math.round(bodyw * 0.9) - (2*bWidth + 2*pPadding))) + "px";
			tipw = $(tooltip).outerWidth(); 
			}
		if (tiph > (Math.round(bodyh * 0.9))) {
			tooltip.style.height = hpop.iequirks ? Math.round(bodyh * 0.9) + "px" : ((Math.round(bodyh * 0.9) - (2*bWidth + 2*pPadding))) + "px";
			tiph = $(tooltip).outerHeight();
			} else if (tooltip.scrollHeight > tooltip.clientHeight) {
			if (!hpop.ie9)
				tooltip.style.height = ($(tooltip).height() + (tooltip.scrollHeight - tooltip.clientHeight)) + "px";
			else
				tooltip.style.height = ($(tooltip).outerHeight() + (tooltip.scrollHeight - tooltip.clientHeight)) + "px";
			}

		} 

   // Now adjust the position, staying as close as possible to click position
		if ((mx + tipw) > (bodyw-40)) {
		tooltip.style.left = (mx -((mx + tipw)-(bodyw-40))) + "px";
		if (parseInt(tooltip.style.left,10) < 10) tooltip.style.left = "10px";
		} else {
			tooltip.style.left=(mx) + "px";
			}


		if ((my + tiph) > (bodyh-40)) {
		tooltip.style.top = (my -((my + tiph)-(bodyh-40))) + "px";
		if (parseInt(tooltip.style.top,10) < 10) tooltip.style.top = "10px";
		} else {
			tooltip.style.top=(my) + "px";
			}
	if (hpop.is_ie && !hpop.ie9 && !tVars.header) {
		tooltip.style.top = (parseInt(tooltip.style.top,10) + $(window).scrollTop()) + "px";
		}

	// If small popup convert style
	if ($(tooltip).width() + $(tooltip).height() < popCfg.smallDim) {
			if (!hpop.isvideo) tooltip.style.cursor = "move";
			hpop.smallPopup = true;
			// tooltip.style.position = !hpop.ie6 ? "fixed" : "absolute";
		} else {
			hpop.smallPopup = false;
			if (!hpop.isvideo) $("#hmpopupdiv").on('mousemove.scrBar',hmpopScrListen);
		}


	// Add the mouse events for controls
 
	if (!hpop.isvideo) {
		addEventX(document, "mousedown", dragger);
		addEventX(document, "mouseup", unDrag);
		}
	addEventX(document, "mousedown", hmClickKill);
	
	// Everything ready, show the popup
	tooltip.style.overflow = "auto";
	$(tooltip).css({display: "none", visibility: "visible"});
	
		$(tooltip).show("fast");

	// Wrap video popups in dark background
	if (hpop.isvideo) {
	$("body").append('<div id="vidbg" style="background-color: black; opacity:0.5;filter:alpha(opacity=50); z-index:800; position: absolute; top: 0; left:0; height: 100%; width: 100%; display:none;"></div>');
	$("#vidbg").fadeIn("slow");
	} 	

	// Prevent independent image selection and moving when dragging popups
	// The drag event is for IE, mousedown for all others
	if (!hpop.ie6 && !hpop.isvideo) {
	$("img").on('mousedown.imagebug', function(e){
		e.preventDefault();
		}).on('drag.imagebug', function(e){
		e.preventDefault();
		});
// Prevent cursor bug on resizing in webkit browsers
		 $("body").on('mousedown.cursorbug', function(e){
		e.preventDefault();
		}).on('drag.cursorbug', function(e){
		e.preventDefault();
		});

	}

	// Bind listener for dragging the border except on small popups and popups containing video in IE
	if (!hpop.smallPopup && !hpop.isvideo && (hpop.ie9 || hpop.ie8)) {

		$("body").on("mousemove.popup", function(e){
		var popPos = $(tooltip).position();
		var popWd = (!hpop.is_ie || hpop.ie6) ? $(tooltip).outerWidth() : $(tooltip).outerWidth();
		var popHt = (!hpop.is_ie || hpop.ie6) ? $(tooltip).outerHeight() : $(tooltip).outerHeight();
		var rBd = ((e.pageX > (popPos.left + (popWd-4))) && (e.pageX < (popPos.left + popWd+8))) ? true : false;
		var bBd = ((e.pageY > (popPos.top + (popHt-4-bWidth))) && (e.pageY < (popPos.top + popHt+8))) ? true : false;
		var corner = ((rBd && (e.pageY > (popPos.top + popHt-10))) || (bBd && e.pageX > (popPos.left + popWd-10))) ? true : false;
		hpop.bdDrag = rBd || bBd ? true : false;

			$("body").css("cursor", function(){
			return corner ? "nw-resize" : rBd && !bBd ? hpop.ewResize : bBd && !rBd ? hpop.nsResize : "";
			});
			if (hpop.is_webkit) {
			$(tooltip).css("cursor", function(){
			var cMove = ((e.pageX > popPos.left) && (e.pageX < (popPos.left + (popWd-20))) && (e.pageY > popPos.top) && (e.pageY < (popPos.top + (popHt-20))));
 			return corner ? "nw-resize" : rBd && !bBd ? hpop.ewResize : bBd && !rBd ? hpop.nsResize : cMove ? "move" : "auto";
 			});

 			}
		});
	} // End border listener

} // End hmDoPopup


// Popup killer
function hmKillPopup(check) {
  	scrBar = false;
	var kPop = document.getElementById("hmpopupdiv");

	// Only kill if popup is open
	if (kPop.style.visibility==="visible") {
	
	    // Fade out video popup and optionally reload to kill 		
		if (hpop.isvideo) {
		if (check) {
		$("#vidbg").fadeOut("slow", function(){
		if (popCfg.killVideo) $(kPop).find("object,video").remove();		
		});
		}
		else {
			$("#vidbg").hide();
			if (popCfg.killVideo) $(kPop).find("object,video").remove();
			}
		}
		// Reset popup and contents containers and key variables
		$(kPop).hide((check ? "fast" : 0), function() {
		$(kPop).off('mousemove.scrBar').css({visibility: "hidden", display: "block", width: "auto", height: "auto", top: "0px", left: "0px", cursor: "default"});
		hpop.isvideo = false;
		hpop.smallPopup = false;
		hpop.bdDrag = false;
		// Remove event handlers to prevent orphans
		// Prevent exceptions for events that are not bound
		try {
		removeEventX(document, "mousedown", hmClickKill);
		removeEventX(document,"mousedown",dragger);
		removeEventX(document,"mouseup",unDrag);
		$("img").off('mousedown.imagebug').off('drag.imagebug');
		$("body").off('mousemove.popup').off('mousedown.cursorbug').off('drag.cursorbug');
		} catch(err) {}
		});
	}
} // End hmKillPopup()


} // End general wrapper function
