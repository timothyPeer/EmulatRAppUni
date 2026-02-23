/*! Help & Manual WebHelp 3 Script functions
Copyright (c) 2015-2020 by Tim Green. All rights reserved. Contact tg@it-authoring.com
*/
	var hmDevice = {};

	hmDevice.agent = navigator.userAgent.toLowerCase();
	hmDevice.platform = navigator.platform.toLowerCase();
	hmDevice.touch = /touch/.test(hmDevice.agent);
	hmDevice.mb = /mobi|mobile/.test(hmDevice.agent);
	hmDevice.ipad = /ipad/.test(hmDevice.platform);
	hmDevice.iphone = /iphone/.test(hmDevice.platform);
	hmDevice.goodandroid = (/android.+?applewebkit\/(?:(?:537\.(?:3[6-9]|[4-9][0-9]))|(?:53[8-9]\.[0-9][0-9])|(?:54[0-9]\.[0-9][0-9]))|android.+?gecko\/[345][0-9]\.\d{1,2} firefox/.test(hmDevice.agent));
	hmDevice.deadandroid = (/android.+?applewebkit\/(?:53[0-6]\.\d{1,2})|firefox\/[0-2]\d\.\d{1,2}/.test(hmDevice.agent));
	hmDevice.android = (/android/.test(hmDevice.agent) && !hmDevice.deadandroid);
	hmDevice.w8desktop = (/windows nt 6\.[2345]/m.test(hmDevice.agent));
	hmDevice.w8metro = (function(){var supported = true; 
				try {new ActiveXObject("htmlfile");} catch(e) {supported = false;} 
				return (!supported && hmDevice.w8desktop);})();
	hmDevice.tb = (/tablet/.test(hmDevice.agent) && (!/trident/.test(hmDevice.agent) || (hmDevice.w8metro && hmDevice.touch)));
	hmDevice.phone = (hmDevice.mb && !hmDevice.ipad && !hmDevice.tb);
	hmDevice.tablet = (hmDevice.ipad || hmDevice.tb || (!hmDevice.phone && hmDevice.android));
	hmDevice.desktop = ((!hmDevice.tablet && !hmDevice.phone));
	hmDevice.device = hmDevice.phone ? "phone" : hmDevice.tablet ? "tablet" : hmDevice.desktop ? "desktop" : "default";
	hmDevice.ppversion = 3.2;
	
	var hmHelp;
	function hmH(helppath,helptopic,helpmode) {
		
		this.showhelp = false;
		this.startupOn = jQuery("div#helpwrapper").is(":visible");
		this.firstLoad = true;
		
		var helpTopic = helptopic, 
			helpPath = helppath,
			helpMode = helpmode,
			helpUrl = document.location.search.substr(1).split("\&"),
			argCount = helpUrl.length,
			helpUrlTopic,
			topicAnchor = "",
			helpLoaded = false,
			helpwrapper = document.getElementById("helpwrapper"),
			helpbutton = document.getElementById("hmHelpButton"),
			showhelpTemp = false,
			hmHelpWindow = null;
		
		// Expose helpwrapper 
		this.helpWrapper = helpwrapper;
		
		
		if (helpUrl[0]  !== "") {
		var doParse = true;

		if (/^hmhelp=/.test(helpUrl[0]) && doParse) {
				topicAnchor = document.location.hash !== "" ? "anchor="+ document.location.hash.substr(1) : "";
				helpUrlTopic = helpUrl[0].substr(7);
				doParse = false; // Prevent multiple calls in a single URL
				showhelpTemp = true;

				if (argCount > 1) {
					for (var x = 1; x < argCount; x++) {
					if (x == 1)
						helpUrlTopic += "?" + helpUrl[x];
					else
						helpUrlTopic += "&" + helpUrl[x];
					}
				if (topicAnchor !== "")
					helpUrlTopic += "&" + topicAnchor; 
				}
				else if (topicAnchor !== "")
					helpUrlTopic += "?" + topicAnchor; 
			}
		}
		
		this.showhelp = showhelpTemp;
		this.mobileHelp = function(hmTopic,hmFunction) {
			if (hmHelpWindow !== null) {
				hmHelpWindow.close();
				hmHelpWindow = null;
			} 
			hmHelpWindow = window.open(helpPath + hmTopic,"hm_mobilehelp","",false);
			
			if (typeof hmFunction == "function")
				setTimeout(function(){
					hmFunction("mobile");
				},500);
		};
		
		this.showHelp = function(hmTopic,hmFunction) {
		
		var callback = false, uParam = false, newTarget = "", frameSrc = "";

		if (typeof hmFunction == "function")
			callback = true;
		
		if (/\&/.test(hmTopic)) {
			uParam = true;
			newTarget = hmTopic.replace(/\&/,"\?").replace(/\#/,"\&anchor=");	
		
		}
		if (helpMode != "desktop") {
			
			if (uParam) {
				this.mobileHelp(newTarget,hmFunction);
			}

			else {
			if (typeof hmTopic !== "undefined" && hmTopic !== null)
			this.mobileHelp(hmTopic,hmFunction);
			
			// If a topic was specified in the URL the standard opener will open that topic the first time if no other topic is called with showHelp()
			
			else if (typeof helpUrlTopic !== "undefined") {				
				this.mobileHelp(helpUrlTopic,hmFunction);
				helpUrlTopic = undefined;
			} else
				this.mobileHelp(helpTopic,hmFunction);
				}
			return;
		}
		
		if (!helpLoaded) {

		if (uParam) {
			frameSrc = helpPath + newTarget;
				}
		else if (typeof hmTopic != "undefined" && typeof helpUrlTopic == "undefined" && hmTopic !== null) {
				frameSrc = helpPath + hmTopic;
				helpTopic = hmTopic;
		} 
		
		else if (typeof helpUrlTopic != "undefined") {
			frameSrc = helpPath + helpUrlTopic;
			helpTopic = helpUrlTopic;
		} else {
			frameSrc = helpPath + helpTopic;
		}
		jQuery('div#helpwrapper').append('<iframe id="hmhelp" class="webhelp" src="'+frameSrc+'" frameborder="0"></iframe>');
		} 
		
		if (helpLoaded && typeof hmTopic != "undefined" && !uParam) {
			xMessage.sendObject("hmhelp",{action: "loadtopic", href: hmTopic, bs: false});
		} else if (uParam) {
			jQuery("iframe#hmhelp").attr("src", helpPath + newTarget);
		}
			helpLoaded = true;

		if (!this.firstLoad || !this.startupOn) {
			if (jQuery('div#helpwrapper').is(":hidden")) {
				jQuery('div#helpwrapper').hide().css('visibility','visible').fadeIn(400,function(){
					if (callback) hmFunction("open");	
				});
				jQuery('.hmHelpToggle').text("Hide Embedded Help");
			}
		else if (typeof hmTopic == "undefined" || hmTopic === null) {
			jQuery('div#helpwrapper').fadeOut(400,function(){
				jQuery(this).css('visibility','hidden').css("display","none");
				if (callback) hmFunction("close");
			});
			jQuery('.hmHelpToggle').text("Show Embedded Help");
			}
		}
		this.firstLoad = false;
		}; // hmHelp.showHelp();
		
		
		
		// Define full window variables in advance
		var FWcurrentCSS = "",
		FWcurrentFrameCSS = "",
		FWfullwindow = false,
		FWcurrentOverflow = jQuery("body").css("overflow"),
		FWcurrentOverflowH = jQuery("html").css("overflow");
		
		// Full window method
		this.doFullWindow = function() {

		if (!helpLoaded) return;
		if (FWfullwindow) {
			jQuery("div#helpwrapper").attr("style",FWcurrentCSS);
			jQuery("iframe#hmhelp").attr("style",FWcurrentFrameCSS);
			jQuery("body").css("overflow",FWcurrentOverflow);
			jQuery("html").css("overflow",FWcurrentOverflowH);
			FWfullwindow = false;
		
		} else {
			// FWcurrentCSS = jQuery("div#helpwrapper").attr("style");
			// alert(jQuery("div#helpwrapper").css("float"));
			FWcurrentCSS = ("position:" + jQuery("div#helpwrapper").css("position") + "; float: " + jQuery("div#helpwrapper").css("float") + "; display: block;" );
			
			FWcurrentFrameCSS = jQuery("iframe#hmhelp").attr("style");
			jQuery("body,html").css("overflow", "hidden");
			jQuery("div#helpwrapper").attr("style","display: block; position: absolute; width: auto; float: none; height: auto; top: 0; left: 0; right: 0; bottom: 0; border-radius: 0;");
			jQuery("iframe#hmhelp").attr("style","");
			window.scrollTo(0,0);
			FWfullwindow = true;
	
			if (this.showhelp) {
				this.showHelp();
				}
			} 
		}; // hmHelp.doFullWindow
		
		// Open the help if there's an URL in desktop mode
		if (this.showhelp && hmDevice.desktop) {
				this.showHelp(helpTopic);
				}
		
	} // hmHelp object constructor

	function initHmHelp(path,topic) {
		
		path = path.slice(-1) == "/" ? path : path + "/";
		// Is jQuery loaded?
		if (!window.jQuery) {
			var scrRef = document.createElement("script");
			scrRef.setAttribute("type","text/javascript");
			scrRef.setAttribute("src", path + "js/jquery.js");
			document.getElementsByTagName("head")[0].appendChild(scrRef);			
		}
		
	// Load and initialize 
	var loadCheck = setInterval(function() {
		if (window.jQuery) {
			clearInterval(loadCheck);
			jQuery(document).ready(function(){
			// Desktop mode with embedded window
			if (hmDevice.desktop) {
					jQuery.ajaxPrefilter( "json script", function(options) {options.crossDomain = true;});
					jQuery.getScript(path + "js/xmessage.js", function( data, textStatus, jqxhr ) {
					xMessage = new xMsg("EMBED PARENT: ");
					hmHelp = new hmH(path,topic,"desktop");
					if (hmHelp.startupOn) {
						hmHelp.showHelp();
						}
					});
			} // End of desktop routine
			// Tablet and phone mode with new window
			else {
				hmHelp = new hmH(path,topic,"mobile");
				if (hmHelp.startupOn) 
					hmHelp.helpWrapper.style.display = "none";
			} // If tablet or mobile
			});
			}
		},50);
	}
