/*! Help & Manual WebHelp 3 Script functions
Copyright (c) 2015-2020 by Tim Green. All rights reserved. Contact tg@it-authoring.com
*/

function xMsg(ogn) {

	// Debugging variable for checking which page the function is running from, intialize with an ID for the page
	var thisOrigin = ogn + ": ",
		xAnchor = /^https??:\/\//im.test(document.location) ? "\?" : "\!";
	
	// We need the domain and protocol when online but not when running locally 
	var docDomain = document.domain;
		if (docDomain !== "" && location.protocol !== "file:")  {
			docDomain = location.protocol + "//" + docDomain + (location.port !== "" ? ":" + location.port : "");
			}
			else {
				docDomain = "*";
			}
	
	// Basic sendmessage function, we don't usually call postMessage directly because the callback needs to be formatted
	function sendmessage(target, message, callback) {
		try {
			var t = typeof target !== "string" ? target : target === "parent" ? parent : target.indexOf("parent.") === 0 ? parent.frames[target.substr(7)].contentWindow : document.getElementById(target).contentWindow;
			if (typeof message == "object") {
				t.postMessage(message,docDomain);
				} else if (typeof message == "string") {
			//console.log("SENDMESSAGE WARNING: Deprecated postMessage with string message: " + message + (callback ? "," + callback : "") + " target: " + target.toString());
			t.postMessage(message + (callback ? "," + callback : ""),docDomain);
				}
			} catch(e) {
			console.log(thisOrigin + "SENDMESSAGE ERROR: " + target + " / " + message + " / " + callback + " / "  + e.message);
			}
		}

	// Handle special starred arguments passed as strings	
	function starArg(a){
		var b;
		a = a.replace(/^\*/,"");
		switch(a){
			case "true":
			return true;
			case "false":
			return false;
			case "null":
			return null;
			case "undefined":
			return undefined;
			default:
			b = parseFloat(a,10);
			if (!isNaN(b))
				return b;
			else
				return a;
		}
	}
	// Execute messaged functions by name without eval or new Function
	function executeFunctionByName(functionName, context ) {
	  var theseargs = typeof arguments[2] == "object" ? arguments[2] : [].slice.call(arguments,2);
	  var namespaces = functionName.split(".");
	  var func = namespaces.pop();
	  for(var i = 0; i < namespaces.length; i++) {
	    context = context[namespaces[i]];
	  }
	  return context[func].apply(context, theseargs);
	}

	// Get and set variables/object properties by name
	function getSetVarByName(varName, varVal) {
		var thisObject = window,
			namespaces = varName.split('.'),
			varname = namespaces.pop();
		for (var i=0; i < namespaces.length; i++)
			thisObject = thisObject[namespaces[i]];
		if (typeof varVal != "undefined") {
			if (typeof varVal == "string")
				thisObject[varname] = starArg(varVal);
			else
				thisObject[varname] = varVal;
			}
		else
			return thisObject[varname];
	}

	var topicCallback = function(t,f,a) {
			var thistopic = t;
			var iterations = 0;
			var docallback = setInterval(function(){
				iterations++;
					if (hmFlags.hmCurrentPage === thistopic) {
						clearInterval(docallback);
				iterations = 0;
						executeFunctionByName(f,window,a);
						//callback();
					} 
				// Quit if we don't get a confirmation after 20 iterations
				if (iterations > 20) {
						clearInterval(docallback);
						iterations = 0;
		}
	
				},3);

		};
		
		// Main listener function for parsing and executing postMessage events
		function doParse(event) {
			event = event || window.event;
			
			// Prevent access from any but the current domain when on a web server
			// Full communication is allowed on local because X-Domain attacks are not possible there
			var currentProtocol = document.location.protocol, func, callback;
			if (currentProtocol.substr(0,4) === "http" && event.origin != docDomain) return;

		/******** Object Data *********/
		// Preferred postMessaging method
		if (typeof event.data == "object") {
			if (!event.data.action) return;
			switch (event.data.action) {
				
				case "setnavlinks":
				hmWebHelp.tocNav(event.data);
				break;
				
				case "setmobnavlinks":
				hmWebHelp.mobNavLinks(event.data);
				break;
				
				case "breadcrumbs":
				hmWebHelp.tocNav({action: "bread", crumbs: event.data});
				break;

				case "loadtopic":
				hmWebHelp.tocNav({action: "set", href: event.data.href, bs: event.data.bs, ac: event.data.ac});
				if (typeof event.data.cbf == 'string') {
					topicCallback(event.data.href,event.data.cbf,event.data.cba);
				}
				break;
				
				case "loadtopicnonav":
				hmWebHelp.tocNav({action: "load", href: event.data.href, bs: event.data.bs });
				if (typeof event.data.cbf == 'string') {
					topicCallback(event.data.href,event.data.cbf,event.data.cba);
				}
				break;
				
				case "callfunction":
				executeFunctionByName(event.data.fn,window,event.data.fa);
				break;
				
				case "href":
				var hrefTarget = event.data.href.replace(/\?anchor\=/,"\#");//lastIndexOf("\?");
				tocSource.findElement(hrefTarget);
				break;
				
				case "bs":
				tocSource.findElement(event.data.bs);
				break;

				// Respond to a setvalue request from another frame
				case "setvalue":
				getSetVarByName(event.data.vn,event.data.vv);
				break;

				// Respond to a getvalue request from another frame
				case "getvalue":
				sendmessage(event.source,{action: "returnvalue", vn: event.data.vn.replace(/\./g,"_"), vv: getSetVarByName(event.data.vn), cbf: event.data.cbf});
				break;

				// Write a received value from the other frame to the returnvalues object
				case "returnvalue":
				executeFunctionByName(event.data.cbf,window,event.data.vv);
				break;

			}
			
		} 
			
		/******** String Data *********/
		// Treat as deprecated
		else if (typeof event.data == "string") 
		console.log("ERROR: xMessage call with deprecated string argument " + event.data);
			
		
			} // doParse()

		if (window.addEventListener) {
			window.addEventListener('message', doParse, false);
		} else {
			(window.attachEvent('onmessage', doParse));
			}

		// Expose object functions as methods for calling 
			return {
				sendObject: sendmessage,
				sendMessage: sendmessage
		};
		
	
		}
