/*! Help+Manual CHM Topic Functions 
For Help+Manual Premium Pack CHM
Copyright (c) 2015-2024 by Tim Green
All Rights Reserved */

// Page functions

var currentPPI = "";

function updateIcons() {
		if (currentPPI > 100) {			
			$("img.menuicon,img.navicon,img.naviconoff").each(function(){
				var iconSrc = $(this).attr("src");
				iconSrc = iconSrc.replace(/\.png$/i,"_h.png");
				$(this).attr("src",iconSrc);
			});
			$("img.menuicon").css({"width": "20px", "height": "20px"});
			$("img.navicon,img.naviconoff").css({"width": "24px", "height": "24px"});
		}
	};

// Load an external script (alternative to Ajax for some scripts)
function loadScript(scriptfile, mode){
 if (mode=="js"){ 
  var ref=document.createElement('script');
  ref.setAttribute("type","text/javascript");
  ref.setAttribute("src", scriptfile);
 }
 else if (mode=="css"){ 
  var ref=document.createElement("link");
  ref.setAttribute("rel", "stylesheet");
  ref.setAttribute("type", "text/css");
  ref.setAttribute("href", scriptfile);
 }
 if (typeof ref!="undefined") {
  document.getElementsByTagName("head")[0].appendChild(ref);
  }
}


// Function to enable ToggleJump for anchor links from the index in CHM only
tVars.intLoc = location.hash;
function pollLocation(startHash) {
  var locNow = startHash;
  var target = locNow.replace(/^#/,"");
  if (locNow.length > 0 && tVars.intLoc != locNow) {
     tVars.intLoc = locNow.toLowerCase();
	 if ($("a[name='"+target+"']").is(":hidden")) {
     toggleJump(false);
	 }
     }
}
 

// Check for effective IE version in CHM
var IEversion = (function() {
    var newIE = false;
    var ua = navigator.userAgent.toLowerCase();
    var re = new RegExp("trident\/([0-9]{1,}[\.0-9]{0,})");
    if (re.exec(ua) != null) {
        newIE = parseFloat(RegExp.$1) >= 4;
    }
	if ((ua.indexOf("msie 6") > -1) && !newIE) return 6;
	else if ((ua.indexOf("msie 7") > -1) && !newIE) return 7;
	else if ((ua.indexOf("msie 7") > -1) && newIE) return 8;
	else return 8;
})();

// Check for CHM viewer in Print mode without our own print function
var chmPrint = (function() {

var lastSlashPos = document.URL.lastIndexOf("/") > document.URL.lastIndexOf("\\") ? document.URL.lastIndexOf("/") : document.URL.lastIndexOf("\\");
return ( document.URL.substring(lastSlashPos + 1, lastSlashPos + 4).toLowerCase() == "~hh" ); 

})();

// Dynamically add CSS before DOM for visually smooth init
function addCss(cssCode) {
var styleElement = document.createElement("style");
  styleElement.type = "text/css";
  if (styleElement.styleSheet) {
    styleElement.styleSheet.cssText = cssCode;
  } else {
    styleElement.appendChild(document.createTextNode(cssCode));
  }
  document.getElementsByTagName("head")[0].appendChild(styleElement);
}

// Initialize noJS elements

if (tVars.header) {
	addCss('html,body {overflow: hidden;}\n' +
	'div#hmheader {position: absolute; overflow: hidden; }\n' +
	'div#idcontent {position: absolute; overflow-x: auto; overflow-y: scroll;visibility: hidden;}\n' +
	'#noScriptNavHead {display: none;}' +
	// CSS for ATOC and topic menus
	'p.autoTOC {color:'+hmatocvars.atoc_linkcolor+';}' +
	'\n.topicmenu ul li {background-color:'+hmatocvars.atoc_bg+';}' +
	'div#atocIcon { visibility: hidden;}' +
	'\n.topicmenu {border-color:'+hmatocvars.atoc_border+'}');
	} else {
	addCss('html,body {overflow: auto;}\n' +
	'div#idcontent {position: static; overflow: hidden;visibility:hidden;}');	
	}

// Check if the current page is a CHM search results topic
function SearchCheck() {
	var foundHilite = false;
    var fontTags = document.getElementsByTagName("FONT");
	if (fontTags.length < 1)
		fontTags = document.getElementsByTagName("font");
    if (fontTags.length > 0) {
      var hStyle = "";
      for (var cCheck = 0; cCheck < fontTags.length; cCheck++) {
        hStyle = fontTags[cCheck].style.cssText.toLowerCase();
        if (hStyle.indexOf("background-color") == 0 || hStyle.indexOf("BACKGROUND-COLOR") == 0) {
	  foundHilite = fontTags[cCheck].parentNode;
          break; 
        }
      }
    }
  return foundHilite;     
}

// Get a cookie

function getCookie(name) {
    var nameEQ = name + "=";
    var ca = document.cookie.split(';');
    for(var i=0;i < ca.length;i++) {
        var c = ca[i];
        while (c.charAt(0)==' ') c = c.substring(1,c.length);
        if (c.indexOf(nameEQ) == 0) return c.substring(nameEQ.length,c.length);
    }
    return null;
}

// Reposition ATOC and topic menu
function menuPos() {
		var headerPos = $("div#hmheader").outerHeight();
     	 $("div.topicmenu").css({"top": headerPos+"px"});
	  }

/* ToggleJump Functions */
// Open all toggles around a target if present
	function toggleCheck(target) {
	    // Array of closed toggles above target point
		var tParents = $(target).parents("div[id^='TOGGLE'][hm\\.state='0']");
		if (tParents.length > 0) {
		// Open the parent toggles from the top down
		for (var i=tParents.length-1;i>-1;i--) {
				var t1 = $(tParents[i]).attr("id");
				var t2 = t1 + "_ICON";
				// Check whether matching icon exists
				if ($("img[id='"+t2+"']").length > 0) {
					HMToggle('expand',t1,t2);
					} else {
					HMToggle('expand',t1);
					}
			}
		// Return pause value for timer
		return tParents.length * 200;
		} else {
		return 0;
		}
	} // toggleCheck(); 
	

// Format printable version 
function doPrint() {
	tVars.printMode = true;
	$("div.topicmenu").hide();
 	$("body").css("visibility","hidden");
		setTimeout(function() {
			var temp = hmAnimate;
			if (temp) hmAnimate = false;
			HMToggleExpandAll(true);
			  $("div#hmheadercontents img, p#breadcrumbs").remove();
			  if (!tVars.printFooter) {$("div.topicfooter").remove();}
			$("td#headerContentsCell").find("h1,span").css("color","#000");
			$("td#headerContentsCell").find("h1").css("padding-left","10px");
			 var header = $("td#headerContentsCell").html();
			 header = '<div style="padding-bottom: 10px;"><form id="resetPrintPage" ><input id="reloadPage" type="button" value="'+tVars.printclose+'" onclick="window.location.reload();"></form>' + header + '</div>';
			 $("div#hmheader, div#autoTocWrapper").remove(); 
			 $("p#breadcrumbs").hide();
			 $("img[id^='TOGGLE']").parent("td").remove();
			 $("div#idcontent").css({"position": "static", "overflow-y": "hidden", "overflow-x": "hidden", "padding-top": 0});
			 $("html").css({overflow: "auto"});
			 $(header).prependTo("div#idcontent");
			 $('a, area').not(':has(> img.image-toggle)').attr({href: "javascript:void(0);", onclick: "void(0);"});
			 hmAnimate = temp;
			 $("body").css("visibility","visible").hide().slideDown("fast");
			// Multithreading browsers won't finish formatting the page correctly if you don't pause before calling print()
			setTimeout(function(){
				print();
				},300);
		},150); // setTimeout
} // end doPrint()

// If target is a toggle open it and any parents
function openTargetToggle(target,context){
   var toggleTarget;
   var headToggle = false;
   if (context == "menu") { 
   	    // Only for ATOC menu
   		toggleTarget = $(target[0]).parent("span:has(a.dropdown-toggle)").find("a.dropdown-toggle").attr("href");
   		if (!toggleTarget) {
			var targetID = $(target[0]).parents("div.dropdown-toggle-body").first().attr("id");
			toggleTarget = $("a.dropdown-toggle[href*='"+targetID+"']");
			toggleTarget = toggleTarget.attr("href");
			}
         } else {  // This is for all other toggleJump() functions
  	          // Check for target in a toggle header without an icon
   	         toggleTarget = $(target[0]).parent("p:has(a.dropdown-toggle)");
   	         toggleTarget = $(toggleTarget).find("a.dropdown-toggle").attr("href");
       		 if (!toggleTarget) {
   	         // Check for target in a toggle header with an icon
  	            toggleTarget = $(target[0]).parent("td").parent("tr").parent().parent("table").parent("div:has(a.dropdown-toggle)");
  	   	        toggleTarget = $(toggleTarget).find("a.dropdown-toggle").attr("href");
   	          }  
			  if (!toggleTarget) {
				  // Check for target in toggle body
				  toggleTarget = $(target[0]).parents("div.dropdown-toggle-body").first();
				  if (toggleTarget) {
					  var tID = toggleTarget.attr("id");
					  toggleTarget = $("a.dropdown-toggle[href*='"+tID+"'");
					  toggleTarget = toggleTarget.attr("href");
				  }
			  }
		   }
   var iconToggle = false;
   var iconTarget = "";
   var toggleParents;
   if (toggleTarget) {
	   
   	    if (toggleTarget.indexOf("ICON") != -1) {
   	    	iconToggle = true;
   	    }
   		toggleTarget = toggleTarget.replace(/^.*?\,\'/, "");
   		toggleTarget = toggleTarget.replace(/\'.*$/,"");
		
		toggleParents = $("div[id='"+toggleTarget+"']").parents("div.dropdown-toggle-body");
		if (toggleParents.length > 0) 
			toggleParents.each(function(){
				var parentID = $(this).attr("id");
				try {
					HMToggle('expand', parentID, parentID + "_ICON");
				} 
				catch(err) {
					HMToggle('expand', parentID);
				}
			});
		
		if (iconToggle) iconTarget = toggleTarget + "_ICON";
   		if (!iconToggle ) {
   			HMToggle('expand',toggleTarget);
   			return true;
   			} else {
   				HMToggle('expand', toggleTarget, iconTarget);
   				return true;
   				} 
   		} else return false;

} // End openTargetToggle


// ToggleJump - jump to anchors in toggles

function findPosY(obj)
{
    var curtop = 0;
    if (obj.offsetParent)
    {
        while (obj.offsetParent)
        {
            curtop += obj.offsetTop
            obj = obj.offsetParent;
        }
    }
    else if (obj.y)
        curtop += obj.y;
    return curtop;
}

// Flash an element on the page.
function flashTarget(obj,repeat,delay) {
	repeat--;
	doFlash();
	function doFlash() {
	setTimeout(function() {
	$(obj).each(function() {
		$(obj).css("visibility","hidden");
		setTimeout(function() {
			repeat--;
			$(obj).css("visibility","visible");
			if (repeat > 0) doFlash();
			},delay)
	});
	},delay);
} // doFlash;

} // flashTarget

function aniScroll(target, mode, noAnimate, tjump) {
	var speed = noAnimate ? 0 : 600;
	var sTarget = tVars.header ? "div#idcontent" : "body";
		if (hmAnimate) {
			hmAnimate = false; 
			var switched = true; 
			}
   	var timer = toggleCheck(target[0]);
	// if (timer < 200) timer = 200;
	timer=0;
	openTargetToggle(target,mode);

	setTimeout(function(timer) {
		if (tjump) $(sTarget).scrollTo(0,0);
		$("div#idcontent").css("visibility","visible");
		$(sTarget).scrollTo($(target), speed, { offset: -16, axis: "y",
			onAfter: function() {
				if (switched) {
				hmAnimate = true; 
				}
			flashTarget($(target[0]).parents("p,tr,h1,h2,h3,h4,h5,h6").first(),3,200);
			}
		});	
		},timer);
}

// ToggleJump for links coming in from outside 
// This won't work with a jQuery load function in Firefox
// We need to preload an event listener to get it faster

function toggleJump(noAnimate) {
if (document.location.hash) {
  var theTargetID = document.location.hash.toString().replace(/\#/, "").toLowerCase(),
	  tempAnimate = hmAnimate,
	  theTarget;
	  if (tempAnimate) hmAnimate = false;
	  HMToggleExpandAll(false);
	  hmAnimate = tempAnimate;
 if ($("a[id='"+theTargetID+"']").length > 0) {
  	theTarget = $("a[id='"+theTargetID+"']");
  	   } else if ($("a[name='"+theTargetID+"']").length > 0) {
  	theTarget = $("a[name='"+theTargetID+"']");
  	   } else {
  	    return false;
  	    }
   aniScroll(theTarget,"page",noAnimate,true);
   return false;
    }
 	
} // End function toggleJump()

// File link fixer
function fLinkParser(fn,type) {
	var ttemp = "";
   if (type == "filelink") {
   fn = fn.substr(fn.indexOf(",")+1,fn.lastIndexOf(",")-1);
   }
   // Replace forward slashes with backslashes
   fn = fn.replace(/\//g,"\\");
   // Remove .\ references for current directory
   if (type != "filelink") ttemp += fn + "\n";
   fn = fn.replace(/^\.\\/,"");
   if (type != "filelink") ttemp += fn + "\n";
    var X, Y, sl, a, ra, stepsUp, path, pathSteps, link, p, w;
    ra = /:/;
    a = location.href.search(ra);
    if (a == 2){
    X = 14;
        } else {
    X = 7;
        }
    sl = "\\";
    Y = location.href.lastIndexOf(sl) + 1;
    stepsUp = fn.match(/\.\.\\/ig);
    path = location.href.substring(X, Y);
    if (stepsUp) {
       fn = fn.replace(/\.\.\\/ig,"");
        pathSteps = path.match(/(\\.*?)(?=\\)/ig);
        p=""; w=pathSteps.length-1;
           for (var q = 0; q < stepsUp.length; q++) {
		   path = path.substr(0,path.lastIndexOf(pathSteps[w])+1);
           w--;
            }
        }
	return "file:\\\\\\" + path + fn;;
}

// Toggle Toggler
function toggleToggles() {
var temp = hmAnimate;
if (HMToggles.length != null) {
	var toggleState = true;
	for (var i = 0; i < HMToggles.length; i++) {
		if (HMToggles[i].getAttribute("hm.state") == "1") {
			toggleState = false;
			break;
			}
		}
	// Don't animate expansion on pages with huge numbers of toggles
	if (HMToggles.length > 20 && temp) hmAnimate = false;
	HMToggleExpandAll(toggleState);
	hmAnimate = temp;
	}		
} // End toggleToggles

function nsheader() {
        if (tVars.printMode) {
		return;
		}
	var maxHeight = 0;
	var headHeight = 0;
	var minHeight = IEversion > 7 ? 53 : 55;
	var titleOffset = ($("div#hmheader").width() + 7) - $("div#atocIcon").position().left;
	headHeight = $("table#topicHeaderTable").outerHeight();
	headHeight = headHeight < minHeight ? minHeight : headHeight;
	if ($("div#hmheader").height() < headHeight)
		$("div#hmheader").css("height", headHeight + "px");
	headHeight = IEversion > 7 ? headHeight : headHeight + 1; 
	$("div#idcontent").css("top",($("div#hmheader").height() + 6) + "px");
	$("div#hmheadercontents").css("padding-right",(titleOffset) + "px");
}

function navIconInit(icon) {
	$(icon).on("mouseover", function(event) {
	if (tVars.hlMode) $(this).addClass("naviconH"); 
		else  $(this).attr("src",$(this).attr("src").replace(/(.*?)\.png$/, "$1_on.png"));
	}).on("mouseout", function(event) {
	if (tVars.hlMode) $(this).removeClass("naviconH");
		else $(this).attr("src",$(this).attr("src").replace(/(.*?)_on\.png$/g, "$1.png"));
	});
	}
	
function initMenu(icon, menu) {
	
	// Show/hide menus 
     $("div#"+icon).on("click."+icon, function(event) {
	 event.stopPropagation();
		// Unbind good housekeeping for menus previously closed by other functions
	 $("div#hmheader").off("mousedown."+icon);
		if ($("div#"+menu).is(":hidden")) {
			menuPos();
			$("div#"+menu).slideDown("fast");
			$("div#unclicker").show();
			
		$("div#hmheader").on("mousedown."+icon, function(event) {
		event.stopPropagation();
		 if ($("div#"+menu).is(":visible")) {
		 $("div#"+menu).slideUp("fast");
		 $(this).css("cursor","");
		 $(this).off("mousedown."+icon);
		 }
		 }).css("cursor","default");
			
			} else {
			if (IEversion > 7) {
			$("div#"+menu).slideUp("fast");
			$("div#unclicker").hide();
			$("div#hmheader").off("mousedown."+icon).css("cursor","");
			}
			} 
     }).css("cursor","pointer");	

}

var repositionToggle = function($obj) {
	if (!tVars.header) return;		
	var $scrollBox = $("div#idcontent"),
		$target = $obj,
		// Get the toggle header
		$header = (function(){ 
			var $temp = $("a.dropdown-toggle[href*="+$obj.attr("id")+"]").parent();
			if ($temp.prop('nodeName').toLowerCase() == "span") {
				$temp = $temp.parent();
				}
			if ($temp.prop('nodeName').toLowerCase() == "span") {
				$temp = $temp.parent();
				}
			if ($temp.prop('nodeName').toLowerCase() == "td") {
				$temp = $("a.dropdown-toggle[href*="+$obj.attr("id")+"]").parents("table").first();
			} else {
				$temp = $("a.dropdown-toggle[href*="+$obj.attr("id")+"]").parents("p").first();
				}
			return $temp;})();
	
	// Abort if toggle header identification fails	
	if (!$header || $header.length == 0 || ($header.prop("nodeName").toLowerCase() != "table" && $header.prop("nodeName").toLowerCase() != "p")) return;
			
	var hdTop = ($header.offset().top - $scrollBox.offset().top) + $scrollBox.scrollTop(),
		toggleHeight = $header.outerHeight() + $target.outerHeight(),
		wdHeight = $scrollBox.height(),
		currentScroll = $scrollBox.scrollTop(),
		toggleOffset = hdTop - currentScroll,
		toggleOverlap = toggleOffset + toggleHeight - wdHeight,
		scrollTarget = Math.round(currentScroll + toggleOverlap) - 8,
		noScroll = false;
		
		if ((toggleOffset + toggleHeight) > wdHeight) {
			if (toggleHeight > wdHeight) {
				$scrollBox.scrollTo(hdTop-10,300,{axis: 'y'});
			} else {
				$scrollBox.scrollTo((scrollTarget+25 > hdTop-10 ? hdTop-10 : scrollTarget+25),300,{axis: 'y'});
			}
		}
}

var HMToggleExpandDropdown = function(obj, value, animate) {
  
  var container = tVars.header ? "div#idcontent" : "body";
  
  if (animate) {
	/* $(obj).stop(); don't stop here */ 
    if (value) {
		
      $(obj).slideDown('fast', function(){
		  repositionToggle($(obj));
	  });
    }
    else {
	  $(obj).animate({ height: 'toggle' }, 'fast', function() {
		if (document.all && !window.opera) { // Avoid collapsing margins bug in IE
	  	  var dummy = $(obj).prev();
	  	  if ($(dummy).outerHeight!=0) dummy = $('<div style="height:1px"></div>').insertBefore(obj);
	  	  else $(dummy).css('display', 'block');
          $(dummy).css('display', 'none');
        } 
  	  });
    } 
  }
  else {
    obj.style.display = (value ? "block" : "none");
  }
}

/* Page Init */
$(document).ready(function(){
	
	var hHt = $("td#headerContentsCell p#breadcrumbs").outerHeight() + $("td#headerContentsCell .p_Heading1").first().outerHeight();
	
	$("body").prepend('<div id="ppitest"></div>');
	currentPPI = document.getElementById('ppitest').offsetWidth;
	$("div#ppitest").remove();
	
	tVars.jumpdone = false;
	
 	// Set class for access to breadcrumbs links
	$("p#breadcrumbs").children("a").addClass("crumbs");
	// Init for topics with headers
	if (tVars.header && !chmPrint) {
	navIconInit("img.navicon");
	
	initMenu("menuIcon","chmMenuWrapper");
	
	// Menu highlight 
	$(document).on("mouseover", "div.topicmenu ul li", function(){
		$(this).css("backgroundColor", hmatocvars.atoc_hoverbgcolor);
		$(this).children().filter("p.autoTOC").css("color", hmatocvars.atoc_hovercolor);
		});
	$(document).on("mouseout", "div.topicmenu ul li", function(){
		$(this).css("backgroundColor", hmatocvars.atoc_bg);
		$(this).children().filter("p.autoTOC").css("color", hmatocvars.atoc_linkcolor);
		});
	
	 $(window).on('resize.menuResize', function() {
	   menuPos();
		});
	   	  // Close menu function
 		$("div#unclicker").on("mousedown", function(event) {
		event.stopPropagation();
		$("div.topicmenu").slideUp("fast");
		$("div#hmheader").css("cursor","");
		$(this).hide();
		});
	
	$(window).on("resize", function() {
	nsheader();
	});
	nsheader();
	} // End of init for topics with headers only

// Get filename of current topic
var thisTopic = document.location.pathname;
thisTopic = thisTopic.replace(/^.*[/\\]|[?#&].*$/,"");

// Set up links on page

// Target H&M-created links and manually-entered anchor links but not href="#" links
// Attach the click function to both normal links and image hotspots
$("a[href^='"+thisTopic+"#'],a[href^='#']:not(a[href='#']),area[href^='"+thisTopic+"#'],area[href^='#']:not(area[href='#'])").click(function(event) {
   localLink=true;
   var localTargetID = $(this).attr("href").replace(/.*?\#/,"").toLowerCase();
   var localTarget = $("a[id='"+localTargetID+"']");
   if (!localTarget.length > 0) localTarget = $("a[name='"+localTargetID+"']");
aniScroll(localTarget,"page",false,false);
return false;
});

 // Setup ATOC
if ($("span[class*='_atoc_'], span[class*='_atocs_']").length >= hmatocvars.atoc_minHeaders && hmatocvars.atoc_show && tVars.header) {
$("div#atocIcon").css("visibility","visible");
if (/^https??:\/\//im.test(document.location)) {
$.getScript('autotoc.js', function(){
	autoTOC();
});
} else {
	loadScript("autotoc.js","js");
	// Execute main function once script is fully loaded
	var loadAtoc = setInterval(function() {
		if (atocLoaded) {
		clearInterval(loadAtoc);
		autoTOC();
		updateIcons();
			}
		},100);	
	}
} else {
	$("div#atocIcon").css("visibility","hidden");
	updateIcons();
}

// Setup sortable tables

if ($("table[id*='_srt_']").length < 1 ) {
	tVars.tablesReady = true; } else
	{
	if (/^https??:\/\//im.test(document.location)) {
	$.getScript('sortable.js', function(){
	sortables_init();
	});
	} else {
		loadScript("sortable.js","js");
		var loadSorter = setInterval(function() {
		if (sortableLoaded) {
		clearInterval(loadSorter);
		sortables_init();
			}
		},100);	
		}
} 

// Code Sample Boxes
var	$copyCodeBoxes = $("span.f_CodeSampleLink");

if ($copyCodeBoxes.length > 0) {
	$copyCodeBoxes.on("click", function(event) {
		
		
		var $codeLink = $(event.target || event.srcElement), $codeLinkHTML = $codeLink.html(),
			$thisTable = $codeLink.parents("table").first(),
			$codeCell = $thisTable.find("td").first(),
			codeCell = $codeCell[0],
			range = document.createRange(),
			sel;
			
			
		range.setStartBefore(codeCell.firstChild);
		range.setEndAfter(codeCell.lastChild);

		sel = window.getSelection();
		sel.removeAllRanges();	
		sel.addRange(range);

		setTimeout(function(){
		
		try {  
			var successful = document.execCommand('copy');  
			
			if (successful) {
				sel.removeAllRanges();
				$codeLink.html("Copied!");
				
				setTimeout(function() {
					$codeLink.html($codeLinkHTML);
					},1000);
				}
			} catch(err) {  
				alert('Sorry, cannot copy to clipboard with this browser!'); 
				} 		
		},300);
		
		});
	};

// Enable relative links to external files (fixes CHM current directory bug and relative path bug)
if (tVars.fixrelativelinks)  {
var relLinks = $("a,area").not("a.topiclink,a.topichotspot,a.webhotspot,a.navlink,a[href^='http'],a[href^='ftp:'],a[href^='mailto:'],a[href^='ms-its'],a[href^='mk\\:\\@MSITStore:'],area[href^='ms-its'],area[href^='mk\\:\\@MSITStore:'],area[href^='http'],a[href^='javascript'],area[href^='javascript'],a[href*='\\#'],area[href*='\\#'],area[href*='.htm'],a.hmHotspotRect[href*='.htm'],a:has(img.image-toggle),a.crumbs,a:not(:contains('\\\'))");
$(relLinks).each(function() {
	var fLink = $(this).attr("href");
	if (fLink) $(this).attr("href",fLinkParser(fLink,"weblink"));
	});

// Convert object links into normal links with absolute paths
$("object[classid='clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11']").not(":has(param[value='ALink']),:has(param[value='KLink'])").each(function() {
	var fLink = $($(this).children("param")[1]).attr("value");
	fLink = fLinkParser(fLink,"filelink");
	var fTrigger = $(this).attr("id");
	$('a[href="javascript:'+fTrigger+'\\.hhclick()"]').attr("href",fLink).attr("target","_blank");
	$(this).remove();
	});
 }

// Activate poll for index anchor links in CHM

if ($("a[name]").length > 0 && $("a.dropdown-toggle").length > 0) {
	// Use hashchange for newer IE
	if ("onhashchange" in window) {
		$(window).on('hashchange', function(event) {
		event.preventDefault();
		toggleJump(false);
		});
	} else { 
		setInterval(function() {pollLocation(location.hash)},300);
		}	
}
 
// Do toggleJump or scroll to search result when the whole page is ready 
(function(){
	var togglesStart, togglesCount;
	var hashCheck = document.location.hash;
	var cycles = 0;
	togglesStart = togglesCount = HMToggles.length;
	var doJump = setInterval(function() {
		 var target = SearchCheck();
		togglesCount = HMToggles.length;
		cycles++;
		if (togglesCount > togglesStart) {
			togglesStart = togglesCount;
			} else {
			tVars.togglesReady = true;
			}
		if (tVars.togglesReady && tVars.tablesReady) {
			$("div#idcontent").css("visibility","visible");
				clearInterval(doJump);
			// Adjust header size
			nsheader();
			// Put focus on scrollable element to activate mouse wheel in CHM
			if (tVars.topicFocus) {
				$("div#idcontent").focus();
				}
		// Is there an anchor to jump to?
			if (hashCheck && hashCheck.length > 0) {
			toggleJump(false);
			} else if (target) {
			var temp = hmAnimate;
			var container = tVars.header ? "div#idcontent" : "body";
			hmAnimate = false;
			HMToggleExpandAll(true);
			$(container).scrollTo(target, 600, {offset: -16, axis: "y",
				onAfter: function() {
				hmAnimate = temp;
				}
			});			
		} 
			
		}
	},300); // End of doJump interval
})();

});
