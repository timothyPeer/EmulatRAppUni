// Handler for post-loading functions from files
	var hmWebHelp = {}, hmxtoggle = true;
hmWebHelp.extFuncs = function(func, args) {	
		var newScript = "";
	if (typeof hmWebHelp.funcs != "object") hmWebHelp.funcs = {};
	
	if (typeof hmWebHelp.funcs[func] == "function") {
		hmWebHelp.funcs[func](args);
	} else {
		// Get name of script and load it
		newScript= "./js/" + func + ".js";
		$.getScript(newScript, function (data, textStatus, jqxhr) {

		if (textStatus === "success" && typeof hmWebHelp.funcs[func] == "function") {
			try {
			hmWebHelp.funcs[func](args);
			} catch(err) {
			// This catches bugs in a semantically correct extFunc
			alert(err);
			}
		} else {
			// This catches source that fails to validate as a function
			alert("External function script " + func + ".js failed to load as a function");
			}
		});
	}
	};
	
	function HMTrackTopiclink() {
		return false;
	}
	
	$(document).ready(function(){
	
	// Topic and web links on the page
	$("div#hmxpopupbody").on(
	"click", 
	"a.topiclink,a.weblink,a.topichotspot,a.webhotspot",
	function(event){
		$(this).attr("target","_blank");
		if ($(this).attr("class").indexOf("topiclink") > -1)
			$(this).attr("href",$(this).attr("href").replace(/\#/,"?anchor="));
		});
	
	$textpopup = $("div#textpopup");
	$textpopup.attr("title","Click outside popup to close. Popups in field-level topics are plain text only.");
	
	var hmClosePopup = function(reset) {
		if (reset)
			$textpopup.html("").attr("style","");
		else
			$textpopup.fadeOut("fast",function(){
			$textpopup.html("").attr("style","");
			});
	};
	
	$textpopup.on("click", function(event){
		event.stopPropagation();
	});
	
	$(document).on("click",function(){
		if ($textpopup.is(":visible"))
			hmClosePopup(false);
	});
	
	// Global load popup function
	hmLoadPopup = function(popObj) {
		var textBody = popObj.hmBody, 
			wnheight = $(window).height(),
			wnwidth = $(window).width(),
			pheight, pwidth,
			spaceabove, spacebelow,
			spaceright, spaceleft,
			fixdims;
		textBody = textBody.replace(/<p.*?>(.*?)<\/p>/ig, "\[\[\$\$\$\]\]$1\[\[\%\%\]\]");
		textBody = textBody.replace(/<\/??.*?>/ig, "");
		textBody = textBody.replace(/\[\[\$\$]](.*?)\[\[\%\%]]/ig, '<p class="ppara">$1</p>');
		hmClosePopup(true);
		$textpopup.html(textBody).show();
		
		// Resize popup
		if ($textpopup.width() > $textpopup.height()) {
			fixdims = ($textpopup.width() + $textpopup.height()) / 2;
			$textpopup.css({"width": fixdims + "px"});
			if ($textpopup.height() > (wnheight - 20))
				$textpopup.css({"width": "95%"});
			}
		// Position poupup
		pwidth = $textpopup.width();
		pheight = $textpopup.height();
		
		// Vertical
		spaceabove = hmWebHelp.popY - 50;
		spacebelow = wnheight - (hmWebHelp.popY);
		if (spaceabove > pheight + 15)
			$textpopup.css("top", (hmWebHelp.popY - (pheight + 12)) + "px");
		else if (spacebelow > pheight + 15)
			$textpopup.css("top", (hmWebHelp.popY + 10) + "px");
		else 
			$textpopup.css("top", "2em");
		
		// Horizontal
		spaceright = wnwidth - (hmWebHelp.popX);
		spaceleft = hmWebHelp.popX;
		
		if (spaceright > pwidth + 15)
			$textpopup.css("left", (hmWebHelp.popX + 12) + "px");
		else 
			$textpopup.css("left", (wnwidth - pwidth + 12) + "px");
	};
	
	// Popup links on the page
	$("div#hmxpopupbody").on(
	"click", 
	"a.popuplink,a.popuphotspot,a.topichotspot[href^='javascript:void']",
	function(event){
		event.preventDefault();
		var target = $(this).attr("data-target");
		hmWebHelp.popX = event.clientX;
		hmWebHelp.popY = event.clientY;
		$.getScript("./jspopups/" + target, function(data, textStatus, jqxhr) {
		});
		
		
		/*var target = $(this).attr("data-target");
		if (target && typeof parent.window.hmXPopup === "object"){
			parent.window.hmXPopup.loadPopup(event, target);
			}
		else 
			alert("Invalid popup link!")*/
		});
	
	
	
	// Dropdown Text Toggles
	
	$("div#hmxpopupbody").on(
	"click", 
	"a.dropdown-toggle",
	function(event){
		event.preventDefault();
		var toggleArgs = {method: "HMToggle", obj: $(this)};
		hmWebHelp.extFuncs("hmDoToggle",toggleArgs);
	});
	$("div#hmxpopupbody").on(
	"click", 
	"img.dropdown-toggle-icon",
	function(event){
		event.preventDefault();
		var toggleArgs = {method: "HMToggleIcon", obj: $(this)};
		hmWebHelp.extFuncs("hmDoToggle",toggleArgs);
	});
	
	// Inline Text Toggles
	$("div#hmxpopupbody").on(
	"click",
	"a.inline-toggle",
	function(event){
	event.preventDefault();
	hmWebHelp.extFuncs("hmDoInlineToggle",$(this));
	});
	
	// Image Toggles

	/*$("div#hmxpopupbody").on(
	"click",
	"img.image-toggle",
	function(event){
	event.preventDefault();
	hmWebHelp.extFuncs("hmImageToggle",($(this).children("img").first()));
	});*/
	
	// Video lightboxes 
	
	$("div#hmxpopupbody").on(
	"click",
	"div.video-lightbox",
	function(event){
		event.preventDefault();
		event.stopPropagation();
		alert("Video lightboxes are not supported in field-level mode. You need to open this page in the main help to view this video.");
	});

		
	});
