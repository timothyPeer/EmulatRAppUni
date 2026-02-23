/*! Help & Manual WebHelp 3 Script functions
Copyright (c) 2015-2020 by Tim Green. All rights reserved. Contact tg@it-authoring.com
*/

// Get the path to the WebHelp folder
var scriptEls = document.getElementsByTagName('script'),
	thisScriptEl = scriptEls[scriptEls.length - 1],
	scriptPath = thisScriptEl.src,
	hmPopupPath = scriptPath.substr(0, scriptPath.lastIndexOf('/js/')+1);
	
//** Main Object **//
var hmXPopup = {};
	hmXPopup.topicTarget = "";
	hmXPopup.topicExtension = "";
	hmXPopup.visitedTopics = {};
var doHmXPopup = function() {
	// Multi-browser preventDefault for non-jQuery functions
	hmXPopup.PreventDefault = function(event) {
	if (typeof event == "undefined" || event === null) return;
	if (event.preventDefault)
		event.preventDefault();
	else
		event.returnValue = false;
	};
	
	if (jQuery("link#popupstyles").length < 1) {
		jQuery("head").append('<link id="popupstyles" rel="stylesheet" type="text/css" href="'+ hmPopupPath + "css/hm_popup.css" + '" />');
	}
	
	jQuery.cachedScript = function( url, options ) {

	  // Use .done(function(script,textStatus){}); for callbacks

	  // Allow user to set any option except for dataType, cache, and url
	  options = jQuery.extend( options || {}, {
	    dataType: "script",
	    cache: true,
	    url: url
	  });
	  return jQuery.ajax( options );
};
	
	// Browser capabilities reference 
	var agent = navigator.userAgent,
		platform = navigator.platform;
		hmXPopup.hmBrowser = {};
		hmXPopup.hmBrowser.touch = !!(('ontouchstart' in window) || ('msmaxtouchpoints' in window.navigator) || ('maxtouchpoints' in window.navigator) || (navigator.maxTouchPoints > 0) || (navigator.msMaxTouchPoints > 0));
		hmXPopup.hmBrowser.nonDeskTouch = !!((hmXPopup.hmBrowser.touch && !/win32|win64/i.test(platform)) || (hmXPopup.hmBrowser.touch && /win32|win64/i.test(platform) && /mobile/i.test(agent)));
	 
		hmXPopup.hmBrowser.eventType = (('onmousedown' in window && !hmXPopup.hmBrowser.nonDeskTouch) ? "mouse" : ('ontouchstart' in window) ? "touch" : ('msmaxtouchpoints' in window.navigator || navigator.msMaxTouchPoints > 0) ? "mstouchpoints" : ('maxtouchpoints' in window.navigator || navigator.maxTouchPoints > 0) ? "touchpoints" : "mouse");
			 switch(hmXPopup.hmBrowser.eventType) {
				case "mouse":
					hmXPopup.hmBrowser.touchstart = "mousedown.startevents";
					hmXPopup.hmBrowser.touchend = "mouseup.endevents";
					hmXPopup.hmBrowser.touchmove = "mousemove.moveevents";
				break;
				case "touch":
					hmXPopup.hmBrowser.touchstart = "touchstart.startevents";
					hmXPopup.hmBrowser.touchend = "touchend.endevents";
					hmXPopup.hmBrowser.touchmove = "touchmove.moveevents";
				break;
				case "mstouchpoints":
					hmXPopup.hmBrowser.touchstart = "MSPointerDown.startevents";
					hmXPopup.hmBrowser.touchend = "MSPointerUp.endevents";
					hmXPopup.hmBrowser.touchmove = "MSPointerMove.moveevents";
				break;
				case "touchpoints":
					hmXPopup.hmBrowser.touchstart = "pointerdown.startevents";
					hmXPopup.hmBrowser.touchend = "pointerup.endevents";
					hmXPopup.hmBrowser.touchmove = "pointermove.moveevents";
				break;
			 }

	// Device capabilities reference
	hmXPopup.hmDevice = {};
	hmXPopup.hmDevice.agent = navigator.userAgent.toLowerCase();
	hmXPopup.hmDevice.platform = navigator.platform.toLowerCase();
	hmXPopup.hmDevice.ipad = /ipad/.test(hmXPopup.hmDevice.platform);
	hmXPopup.hmDevice.iphone = /iphone/.test(hmXPopup.hmDevice.platform);
	hmXPopup.hmDevice.winphone7 =  (/windows phone os [7]\.\d\d??; .*?trident\/[5]\.\d\d??; iemobile\/9/.test(hmXPopup.hmDevice.agent));
	hmXPopup.hmDevice.winphone = (/windows phone [89]\.\d\d??; .*?trident\/[67]\.\d\d??;.*?touch; /.test(hmXPopup.hmDevice.agent));
	hmXPopup.hmDevice.w8desktop = (/windows nt 6\.[2345]|windows nt 10\.[0123456]/m.test(hmXPopup.hmDevice.agent));
	hmXPopup.hmDevice.msBrowser = (/edge|trident/m.test(hmXPopup.hmDevice.agent));
	hmXPopup.hmDevice.w8metro = (function(){var supported = true; 
				try {new ActiveXObject("htmlfile");} catch(e) {supported = false;} 
				return (!supported && hmXPopup.hmDevice.w8desktop && hmXPopup.hmDevice.msBrowser);})();
	hmXPopup.hmDevice.touch = !!(('ontouchstart' in window && !window.opera) || ('msmaxtouchppoints' in window.navigator) || ('maxtouchppoints' in window.navigator) || (navigator.maxTouchPoints > 0) || (navigator.msMaxTouchPoints > 0));
	// Treat all Windows 8 devices as desktops unless in metro mode with touch
	hmXPopup.hmDevice.tb = (/tablet/.test(hmXPopup.hmDevice.agent) && (!/trident/.test(hmXPopup.hmDevice.agent) || (hmXPopup.hmDevice.w8metro && hmXPopup.hmDevice.touch)));
	hmXPopup.hmDevice.goodandroid = (/android.+?applewebkit\/(?:(?:537\.(?:3[6-9]|[4-9][0-9]))|(?:53[8-9]\.[0-9][0-9])|(?:54[0-9]\.[0-9][0-9]))|android.+?gecko\/[345][0-9]\.\d{1,2} firefox/.test(hmXPopup.hmDevice.agent));
	hmXPopup.hmDevice.deadandroid = (/android.+?applewebkit\/(?:53[0-6]\.\d{1,2})|firefox\/[0-2]\d\.\d{1,2}/.test(hmXPopup.hmDevice.agent));
	hmXPopup.hmDevice.android = (/android/.test(hmXPopup.hmDevice.agent) && !hmXPopup.hmDevice.deadandroid);
	hmXPopup.hmDevice.mb = /mobi|mobile/.test(hmXPopup.hmDevice.agent);
	
	/* Main Device References */
	hmXPopup.hmDevice.phone = (hmXPopup.hmDevice.mb && !hmXPopup.hmDevice.ipad && !hmXPopup.hmDevice.tb);
	hmXPopup.hmDevice.tablet = (hmXPopup.hmDevice.ipad || hmXPopup.hmDevice.tb || (!hmXPopup.hmDevice.phone && hmXPopup.hmDevice.android));
	hmXPopup.hmDevice.aspectRatio = (screen.height / screen.width > 1 ? screen.height / screen.width : screen.width / screen.height).toFixed(2);
	hmXPopup.hmDevice.narrowTablet = (hmXPopup.hmDevice.tablet && hmXPopup.hmDevice.aspectRatio > 1.4);
	hmXPopup.hmDevice.desktop = ((!hmXPopup.hmDevice.tablet && !hmXPopup.hmDevice.phone));	
	hmXPopup.hmDevice.device = hmXPopup.hmDevice.phone ? "phone" : hmXPopup.tablet ? "tablet" : hmXPopup.hmDevice.desktop ? "desktop" : "default";
	
	// Get the parent container
	var $popparent = jQuery("body");
	
	// Create the necessary components
	$popparent.prepend('<div id="hmpopupbox" class="hmpopup"><div id="hmpopuptitlebar" class="hmpopup"><div id="hmpopuptitle"  class="hmpopup"><p class="hmpopup">This is the title bar</p></div><div id="hmclosepopup" class="hmpopup"><span>X</span></div></div><div id="hmpopupbody"><iframe id="hmXPopupFrame" src="" class="hmpopup" frameborder="0"></iframe></div></div>');

	$popparent.prepend('<div id="dragsurface" style="display: none; position: fixed; top: 0; right: 0; bottom: 0; left: 0; background-image: url(\''+hmPopupPath+'images/spacer.gif\')" />');

	// Get the JQ variables once for faster reuse
	hmXPopup.$popup = jQuery("div#hmpopupbox");
	hmXPopup.$popuptitle = jQuery("div#hmpopuptitle > p");
	hmXPopup.$popupheader = jQuery("div#hmpopuptitlebar");
	hmXPopup.$popupdragger = jQuery("div#hmpopuptitle");
	hmXPopup.$dragsurface = jQuery("div#dragsurface");
	hmXPopup.$popupbody = null;
	hmXPopup.$popupscroller = null;
	hmXPopup.$popupwindow = null;
	hmXPopup.$popframe = jQuery("iframe#hmXPopupFrame");
	hmXPopup.refPath = hmPopupPath; //"./";
	hmXPopup.currentPopup = "";
	hmXPopup.clickX = 0; 
	hmXPopup.clickY = 0;
	hmXPopup.relposX = 0;
	hmXPopup.relposY = 0;

	hmXPopup.hmResizePopup = function() {
		var popBodyWidth = 0, popBodyHeight = 0,
			windowWidth = function(){return jQuery(window).width();},
			wW = 0, pW = 0,
			windowHeight = function(){return jQuery(window).height();},
			wH = 0, pH = 0,
			newWidth = 0, newHeight = 0,
			verticalScroller = function() {return hmXPopup.$popupscroller.height() > hmXPopup.$popupwindow.height();},
			horizontalScroller = function() {return hmXPopup.$popupscroller.width() > hmXPopup.$popupwindow.width();};
			popBodyWidth = hmXPopup.$popupbody.outerWidth(true);
			popBodyHeight = hmXPopup.$popupbody.outerHeight(true);
			
			// Horizontal scrollbar?
			if (horizontalScroller()) {
				hmXPopup.$popup.width((hmXPopup.$popupscroller.width() + 10) + "px");
			}
			
			// Adjust both dimensions for vertical scrollbar
			
			if (verticalScroller()) {
				newWidth = hmXPopup.$popup.width();
				newHeight = hmXPopup.$popup.height();
				do {
					newWidth+=5; newHeight+=5;
					hmXPopup.$popup.width(newWidth + "px");
					hmXPopup.$popup.height(newHeight + "px");
				} while (verticalScroller() && (newWidth < windowWidth() * 0.5));
					
			if (horizontalScroller()) {
				hmXPopup.$popup.width((hmXPopup.$popupscroller.width() + 5) + "px");
			}
				
			if (verticalScroller()) {					 
				hmXPopup.$popup.height((hmXPopup.$popupheader.height() + hmXPopup.$popupscroller.height() + 2) + "px");
				}
			}
			
		// Now position the popup
		
		wW = windowWidth()-5; pW = hmXPopup.$popup.outerWidth();
		wH = windowHeight()-5; pH = hmXPopup.$popup.outerHeight();
		
		// Does it fit in the standard position (just below and right of click position)?
		if ((pH + hmXPopup.relposY + 15 < wH-5) && (pW + hmXPopup.relposX  + 30 < wW-5)) {
			hmXPopup.clickY+=15;
			hmXPopup.clickX+=30;
		} else {
		 
		 // Vertical: Move up minimum amount to fit
		if (pH + hmXPopup.relposY + 15 > wH-5) {
			 
			 hmXPopup.clickY = jQuery(document).scrollTop() + (wH-pH);
			 hmXPopup.clickY = hmXPopup.clickY < 0 ? 5 : hmXPopup.clickY;
		 } else hmXPopup.clickY+=15;
		 
		 // Horizontal: Move left to fit
		 if (pW + hmXPopup.relposX  + 30 > wW-5) {
			 
			 // Fits left of the click point?
			 if (pW+5 < hmXPopup.relposX) {
				 hmXPopup.clickX = jQuery(document).scrollLeft() + (wW-pW);
			 } else { 
				hmXPopup.clickX = (jQuery(document).scrollLeft() + wW) - (pW+5);
			 }
			 hmXPopup.clickX = hmXPopup.clickX < 0 ? 5 : hmXPopup.clickX;
		 } else hmXPopup.clickX+=30;
		 
		}
	
		hmXPopup.$popup.css({"top": (hmXPopup.clickY) + "px", "left": (hmXPopup.clickX) + "px"});
		
	};
	
	// Global load popup function to execute from the JSON file
	hmLoadPopup = function(popObj) {
		hmXPopup.noresize = false;
		var sourceTest = hmXPopup.$popframe.attr("src");
		if (sourceTest === "" || /_hmXtopic.htm$/.test(sourceTest)){
			hmXPopup.$popframe.attr("src", hmXPopup.refPath + "_hmXpopup.htm");
				}
		var loadPopper = setInterval(function(){
			
			hmXPopup.$popupbody = hmXPopup.$popframe.contents().find("div#hmxpopupbody");
		if (hmXPopup.$popupbody.length > 0) {
			clearInterval(loadPopper);
			
			hmXPopup.$popupscroller = hmXPopup.$popframe.contents().find("body");
			hmXPopup.$popupwindow = jQuery("div#hmpopupbody");
			hmXPopup.$popupbody.html(popObj.hmBody);
			hmXPopup.$popuptitle.html(popObj.hmTitle);
			hmXPopup.$popup.css({"height": "3.6rem", "width": "20rem"});
			hmXPopup.$popup.show();
			hmXPopup.hmResizePopup();
			
			/*jQuery("div#unclicker").show().on("click",function(){
				hmXPopup.closePopup();
			});*/
		if (hmXPopup.hmDevice.desktop && !hmXPopup.noresize) {
			jQuery("body").on("mousemove.resizepopuplistener",function(event){
			var ev = event.originalEvent;
			hmXPopup.resizeListener(ev);
			});
			jQuery("body").on("mousedown.resizepopup",function(event){
				var e = event.originalEvent;
				hmXPopup.startResizePopup(e);
			});
		} // Resizable for desktop browsers
		
		} // Actual load routine
		},30); // Poller for popup content present
	};
	
	// Work out the topic extension that is in use
	var setTopicExtension = function(obj) {
		var ext = "";
		if (obj.hmPrevLink !== "")
			ext = obj.hmPrevLink;
		else if (obj.hmNextLink !== "")
			ext = obj.hmNextLink;
		else if (obj.hmParentLink !== "")
			ext = obj.hmParentLink !== "";
		if (ext === "") 
			hmXPopup.topicExtension = false;
		else 
			hmXPopup.topicExtension = ext.substr(ext.lastIndexOf("\."));
		};
	
	// Global load topic function to execute from the JSON file	
	hmLoadTopic = function(topObj) {
		
		hmXPopup.noresize = false;
		var topicHeader = "";
		if (hmXPopup.topicExtension === "")
			setTopicExtension(topObj);
		if (hmXPopup.topicExtension) {
			hmXPopup.topicTarget = hmPopupPath + hmXPopup.currentPopup.replace(/\.js$/,hmXPopup.topicExtension);
			topicHeader = "<p class='linkheader'><a href='"+hmXPopup.topicTarget+"' target='_blank'>Click here to open this topic in a full help window</a></p>";
		}
		var sourceTest = hmXPopup.$popframe.attr("src");
		if ( sourceTest === "" || /_hmXpopup.htm$/.test(sourceTest))
			hmXPopup.$popframe.attr("src", hmXPopup.refPath + "_hmXtopic.htm");
		var loadPopper = setInterval(function(){
			hmXPopup.$popupbody = hmXPopup.$popframe.contents().find("div#hmxpopupbody");
		if (hmXPopup.$popupbody.length > 0) {
			clearInterval(loadPopper);
			hmXPopup.$popupscroller = hmXPopup.$popframe.contents().find("body");
			hmXPopup.$popupwindow = jQuery("div#hmpopupbody");
			hmXPopup.$popupbody.html(topicHeader + topObj.hmBody);
			hmXPopup.$popuptitle.html(topObj.hmTitle);
			hmXPopup.$popup.css({"height": "3.6rem", "width": "20rem"});
			hmXPopup.$popup.show();
			hmXPopup.hmResizePopup();
			
		if (hmXPopup.hmDevice.desktop && !hmXPopup.noresize) {
			jQuery("body").on("mousemove.resizepopuplistener",function(event){
			var ev = event.originalEvent;
			hmXPopup.resizeListener(ev);
			});
			jQuery("body").on("mousedown.resizepopup",function(event){
				var e = event.originalEvent;
				hmXPopup.startResizePopup(e);
			});
		} // Resizable for desktop browsers
		
		// Image Toggles
	hmXPopup.$popupbody.find("img.image-toggle").parent("a").on("click",function(event){
		event.preventDefault();	
		//alert("Image toggles not supported in field-level mode")
		document.getElementById("hmXPopupFrame").contentWindow.hmWebHelp.extFuncs("hmImageToggle",(jQuery(this).children("img").first()));
		});
		
		
		} // Actual load routine
		},30); // Poller for popup content present
	};
	
	hmXPopup.closePopup = function() {

		if (hmXPopup.$popup.is(":hidden"))
			return;
		hmXPopup.$popup.fadeOut(300);
		
		// Unbind all the resizing on the desktop
		if (hmXPopup.hmDevice.desktop) {
			jQuery("body").off(".resizepopup").off(".resizepopuplistener");
			hmXPopup.$dragsurface.off(".resizepopup");
		}
		
		// Kill iframe videos before closing to prevent youtube crashes
		//hmXPopup.$popupbody.find("iframe").attr("src","");
		// hmXPopup.$popupbody.html("");
		
		// Kill any video iframes and objects to prevent hangovers and crashes
		hmXPopup.$popupbody.find("iframe").attr("src","");
		var $videoBits = hmXPopup.$popupbody.find("object,embed,param");
		if ($videoBits.length > 0) {
			// In IE the only a reload gets rid of the buffered video object
			if (/trident|edge/i.test(window.navigator.userAgent)) {
				document.location.reload();
			}
			else {
				$videoBits.attr("data","").attr("src","").attr("value","").remove();
			}
		}
		
		// Kill any image toggle boxes
		hmXPopup.$popupscroller.find("div#imagetogglebox").remove();
		
		// Clear the inline style settings of the popup and contents of the container
		hmXPopup.$popup.attr("style","");
		hmXPopup.$popupbody.html("");
		// Clear up any drag residue
		endDrag();
	};

	var fixTarget = function(t) {
		if (/\.js$/i.test(t)) {
			return t.toLowerCase();
		} else {
			return t.toLowerCase() + ".js";
		}
	};

	hmXPopup.loadPopup = function(e, thisPopup, refPath){
		
		var cacheTopic = thisPopup,
			loadThis = "";

		if (Object.keys(hmXPopup.visitedTopics).length > 300)
			hmXPopup.visitedTopics = {};

		hmXPopup.clickX = e.pageX;
			hmXPopup.relposX = hmXPopup.clickX - jQuery(document).scrollLeft();
		hmXPopup.clickY = e.pageY;
			hmXPopup.relposY = hmXPopup.clickY - jQuery(document).scrollTop();
		if (typeof refPath == "undefined") refPath = hmPopupPath + "jspopups/";
		hmXPopup.currentPopup = thisPopup;
		loadThis = refPath + thisPopup;

	if (hmXPopup.visitedTopics.hasOwnProperty(cacheTopic)){
		$.cachedScript(loadThis).done(function(script,textStatus){
		});
		} else {
		jQuery.getScript(loadThis, function(data, textStatus, jqxhr) {
			hmXPopup.visitedTopics[cacheTopic] = true;
		});
		}

		
	};
	// Bind the events for popup links on the page 
	jQuery("a.hmpopuplink").on("click",function(event){
			event.preventDefault();
			hmXPopup.loadPopup(event,fixTarget(jQuery(this).attr("data-target")), hmPopupPath + "jspopups/");
		});
	jQuery("a.hmtopiclink").on("click",function(event){
			event.preventDefault();
			hmXPopup.loadPopup(event,fixTarget(jQuery(this).attr("data-target")), hmPopupPath + "jstopics/");
		});
		
	// Popup closing routines
	jQuery("div#hmclosepopup").on(hmXPopup.hmBrowser.touchstart,hmXPopup.closePopup);
	
	jQuery(document).keyup(function(e) {
     if (e.keyCode == 27) { 
		hmXPopup.closePopup();
		}
	});
	
	/*** Draggable Popups ***/
	
	// General variables
	var startTime = 0,
		dragTime = 0,
		dragcount = 0,
		// $dragsurface = $dragsurface = jQuery("div#dragsurface"),
		oldX,
		oldY,
		oldLeftPos, 
		oldTopPos;
		
	
	// Get the type of interaction event from user
	var EventType = function(e) {
		if (e.pointerType == "mouse" || e.pointerType == 4)
			return "mouse";
		else if (e.pointerType == "touch" || e.pointerType == 2 || e.pointerType == "pen" || e.pointerType == 3)
			return "touch";
		else if (/^mouse/i.test(e.type)) 
			return "mouse";
		else if (/^touch/i.test(e.type) || /^pen/i.test(e.type)) 
			return "touch";
		else return "none";
		};
	
	// Perform this at the end of a drag operation
	var endDrag = function(e) {
		dragTime = new Date().getTime() - startTime;
		if (dragTime < 200) {
			}
		hmXPopup.$popupdragger.off(".endevents");
		hmXPopup.$popupdragger.off(".moveevents");
		hmXPopup.$dragsurface.off(".endevents");
		hmXPopup.$dragsurface.off(".moveevents");
		hmXPopup.$dragsurface.hide().css("cursor","default");
		hmXPopup.PreventDefault(e);
		};
			// Drag action
	var performDrag = function(e) {
		
		dragTime = new Date().getTime() - startTime;
		if (dragTime < 50) {
			return;
		}
		
		
		var touchobj;
		if (typeof e.changedTouches != 'undefined') { 
				touchobj = e.changedTouches[0];
			} else {
				touchobj = e;
			}
		
		// Only move once every x events on mobile for lower processor load 
		dragcount++;
		if ( hmXPopup.hmDevice.desktop || dragcount > 2 ) {
		dragcount = 0;
		var moveX = (!(document.all && !window.opera)) ? touchobj.pageX - oldX : touchobj.clientX - oldX;
		var moveY = (!(document.all && !window.opera)) ? touchobj.pageY - oldY: touchobj.clientY - oldY;
		hmXPopup.$popup.css({"left": (oldLeftPos + moveX) + 'px'});
		hmXPopup.$popup.css({"top": (oldTopPos + moveY) + 'px'});
		}
	};
	// Triggered at beginning of a drag
    var startDrag = function(e) {
		hmXPopup.PreventDefault(e);
		startTime = new Date().getTime();
		var touchobj;
		if (typeof e.changedTouches != 'undefined') 
			touchobj = e.changedTouches[0];
		else 
			touchobj = e;
		oldX = (!(document.all && !window.opera)) ? touchobj.pageX : touchobj.clientX;
		oldY = (!(document.all && !window.opera)) ? touchobj.pageY : touchobj.clientY;
		oldLeftPos = hmXPopup.$popup.position().left;
		oldTopPos = hmXPopup.$popup.position().top;

		// Activate the drag surface overlay
		if (hmXPopup.hmBrowser.touch || hmXPopup.hmDevice.winphone || EventType(e) == "mouse") {
		hmXPopup.$dragsurface.css("cursor","all-scroll").show();
		hmXPopup.$dragsurface.on(hmXPopup.hmBrowser.touchmove, function(event) {
			var ev = event.originalEvent; 
			performDrag(ev);
			});
		hmXPopup.$dragsurface.on(hmXPopup.hmBrowser.touchend, function(event) {
			var ev = event.originalEvent; 
			endDrag(ev);
			});
		} 
		
		hmXPopup.$popupdragger.on(hmXPopup.hmBrowser.touchmove, function(event) {
			var ev = event.originalEvent; 
			performDrag(ev);
			});
		hmXPopup.$popupdragger.on(hmXPopup.hmBrowser.touchend, function(event) {
			var ev = event.originalEvent; 
			endDrag(ev);
			});

	};

	hmXPopup.$popupdragger.on(hmXPopup.hmBrowser.touchstart, function(event) {
		var ev = event.originalEvent; 
		startDrag(ev);
		});	

if (hmXPopup.hmDevice.desktop) {
	
	hmXPopup.resizeListener = function(e) {
	// var e = event.originalEvent,
	 var popPos = hmXPopup.$popup.position(),
	 popWd = hmXPopup.$popup.outerWidth(),
	 popHt = hmXPopup.$popup.outerHeight(),
	 bWidth = 2;
	 hmXPopup.ewResize = "ew-resize";
	 hmXPopup.nsResize = "ns-resize";
	// Body cursor for resizing, based on proximity to left and right borders of popup
	var rBd = ((e.pageX > (popPos.left + (popWd-4))) && (e.pageX < (popPos.left + popWd+8)) && (e.pageY > popPos.top) && (e.pageY < popPos.top + popHt+8));
	var bBd = ((e.pageY > (popPos.top + (popHt-8-bWidth))) && (e.pageY < (popPos.top + popHt+10)) && (e.pageX < (popPos.left + popWd+10)) && (e.pageX > popPos.left+4));
	var corner = ((rBd && (e.pageY > (popPos.top + popHt-10))) || (bBd && e.pageX > (popPos.left + popWd-10)));
	hmXPopup.bdDrag = rBd || bBd;
	jQuery("body").css("cursor",function(){
	return corner ? "nw-resize" : rBd && !bBd ? hmXPopup.ewResize : bBd && !rBd ? hmXPopup.nsResize : "auto";
	});
	}; // resize listener
	

	// Deselect during drag
	hmXPopup.deSelect = function(){
		if (window.getSelection){
			window.getSelection().removeAllRanges();
		}
		else if (document.selection){
			document.selection.empty();
		}
	return false;
	}; // End deSelect()
	
	hmXPopup.doResizePopup = function(event,direction) {
		hmXPopup.deSelect();
		var moveX = event.pageX - hmXPopup.resizeX,
			moveY = event.pageY - hmXPopup.resizeY,
			newX = hmXPopup.popdims.w + moveX,
			newY = hmXPopup.popdims.h + moveY;
			
			switch (direction) {
				case "horizontal":
				newY = hmXPopup.popdims.h;
				break;
				case "vertical":
				newX = hmXPopup.popdims.w;
				break;
			}
			if (newY < 50) newY = 50;
			if (newX < 200) newX = 200;
			hmXPopup.$popup.css({"width": newX + "px", "max-width": newX + "px", "height": newY + "px", "max-height": newY + "px"});
	};
	
	hmXPopup.endResizePopup = function(event) {
		// jQuery("body").off("mousemove.resizepopup");
		hmXPopup.$dragsurface.off("mousemove.resizepopup");
		jQuery("body").off("mouseup.resizepopup");
		hmXPopup.$popupbody.css("overflow","auto");
		jQuery("body").css("cursor","default");
		hmXPopup.$dragsurface.css("cursor","default").hide();
	};
	
	hmXPopup.startResizePopup = function(e) {
		var thisCursor = jQuery("body").css("cursor");
		function initialize(direction) {
			hmXPopup.resizeX = e.pageX;
			hmXPopup.resizeY = e.pageY;
			hmXPopup.$popupbody.css("overflow","hidden");
			hmXPopup.popdims = {};
			hmXPopup.popdims.w = hmXPopup.$popup.width();
			hmXPopup.popdims.h = hmXPopup.$popup.height();
			jQuery("body").on("mouseup.resizepopup", function(event) {
			var e = event.orginalEvent;
			hmXPopup.endResizePopup(e);
			});
			hmXPopup.$dragsurface.css("cursor",thisCursor);
			jQuery("body").css("cursor",thisCursor);
			hmXPopup.$dragsurface.show().on("mousemove.resizepopup", function(event){
			var e = event.originalEvent;
			hmXPopup.doResizePopup(e,direction);
			});
		}
		switch (thisCursor) {
			case "ew-resize":
			initialize("horizontal");
			break;
			case "nw-resize":
			initialize("diagonal");
			break;
			case "ns-resize":
			initialize("vertical");
			break;
		}
	};
	
} // If Desktop for resizable popups
	
}; // doHMXPopup

if (typeof jQuery === "undefined") {
	var jQ = document.createElement('script');
	jQ.setAttribute('type','text/javascript');
	jQ.setAttribute('src', hmPopupPath + 'js/jquery.js');
	document.getElementsByTagName("head")[0].appendChild(jQ);
	var jQLoader = setInterval(function(){
		if (typeof jQuery === 'function') {
			clearInterval(jQLoader);
			jQuery(document).ready(function(){
			doHmXPopup();
			});
		} 
	},50);
} else {
	jQuery(document).ready(function(){
	doHmXPopup();
	});
}	
