/*! Table sorting code based on scripts by Stuart Langridge 
http://www.kryogenix.org/code/browser/sorttable/
and Joost de Valk http://www.joostdevalk.nl/code/sortable-table/
Original code in these scripts copyright (c) 1997-2008 by Stuart 
Langridge and Joost de Valk and released under the MIT License.

Modified version for Help+Manual Premium Pack CHM
Copyright (c) 2015-2024 by Tim Green

Modified and extended for use in Help+Manual by Tim Green. This
version is a complete rewrite for use in Help+Manual with a large
number of major changes and new features and it will not work outside
of Help+Manual projects. For use in your own scripts see the original 
versions on Stuart Langridge's and Joost de Valk's websites. */
var SORT_COLUMN_INDEX;
var thead = false;
var altTables = new Array();

/* if (!window.opener) {
	var sortVars = parent.window.sortVars } else {
	var sortVars = window.opener.window.sortVars;
	}
*/
var sortID = "_srt_";
var europeandate = sortVars["europeandate"];
var germanNumbers = sortVars["germanNumbers"];
var image_path = sortVars["imagePath"];
var image_up = sortVars["image_up"];
var image_down = sortVars["image_down"];
var image_none = sortVars["image_none"];
var image_empty = sortVars["image_empty"];
var sort_tip = sortVars["sort_tip"];
var sortUmlauts = sortVars["umlauts"];
var bottomrow = "_bottomrow_";
var sortablecol = "_sortable_";
var defaultsort = "_defaultsort_";
var defaultsortD = "_defaultsortd_";
var customicons = "_cicons-";
var noalternate = "_noalt_";
// Clean strings of accented characters for sorting
var makeSortString = (function() {
var translate_re  = /%C3%A4|%C3%B6|%C3%BC|%C3%84|%C3%96|%C3%9C|%C3%A1|%C3%A0|%C3%A2|%C3%A9|%C3%A8|%C3%AA|%C3%BA|%C3%B9|%C3%BB|%C3%B3|%C3%B2|%C3%B4|%C3%81|%C3%80|%C3%82|%C3%89|%C3%88|%C3%8A|%C3%9A|%C3%99|%C3%9B|%C3%93|%C3%92|%C3%94|%C3%9F/g;
var translate = {
   "%C3%A4": "a", "%C3%B6": "o", "%C3%BC": "u",	
   "%C3%84": "A", "%C3%96": "O", "%C3%9C": "U",
   "%C3%A1": "a", "%C3%A0": "a", "%C3%A2": "a",
   "%C3%A9": "e", "%C3%A8": "e", "%C3%AA": "e",
   "%C3%BA": "u", "%C3%B9": "u", "%C3%BB": "u",
   "%C3%B3": "o", "%C3%B2": "o", "%C3%B4": "o",
   "%C3%81": "A", "%C3%80": "A", "%C3%82": "A",
   "%C3%89": "E", "%C3%88": "E", "%C3%8A": "E",
   "%C3%9A": "U", "%C3%99": "U", "%C3%9B": "U",
   "%C3%93": "O", "%C3%92": "O", "%C3%94": "O",
   "%C3%9F": "s"
	};
  return function(s) {
    return ( s.replace(translate_re, function(match) { 
      return translate[match]; 
    }) );
  }
})();

// String trimmer
function trim(s) {
	return s.replace(/^\s+|\s+$/g, "");
}

// Function for getting the value of a specific stylesheet class attribute
function getCssAttrib(className,classAttrib) {

	 var sheetRules;
	 var theValue;
	 className = "." + className;

// Get stylesheet mode of browser
    if (document.styleSheets[0]['rules']) {
	  sheetRules = 'rules';
	 } else if (document.styleSheets[0]['cssRules']) {
	  sheetRules = 'cssRules';
	 } else {
	  // Retarded browser, give up...
        return false;
	 }

for (var sheets = 0; sheets < document.styleSheets.length; sheets++){

	  for (var thisSheet = 0; thisSheet < document.styleSheets[sheets][sheetRules].length; thisSheet++) {
	   if (document.styleSheets[sheets][sheetRules][thisSheet].selectorText == className) {
	   	   // alert("Class attrib searched for is: "+classAttrib);
	    	if(document.styleSheets[sheets][sheetRules][thisSheet].style[classAttrib]){
	    		theValue = document.styleSheets[sheets][sheetRules][thisSheet].style[classAttrib];
	    		// alert("Found class attrib: "+theValue);
	    		return theValue;
	    		}
	    	}
		}
	} // End stylesheet scan
	return false; // Style or attribute not found
} // End getCssAttrib()

// Get plain text of element
function ts_getInnerText(el) {
	if (typeof el == "string") return el;
	if (typeof el == "undefined") { return el };
	if (el.innerText) return el.innerText;	//Not needed but it is faster
	var str = "";

	var cs = el.childNodes;
	var l = cs.length;
	for (var i = 0; i < l; i++) {
		switch (cs[i].nodeType) {
			case 1: //ELEMENT_NODE
				str += ts_getInnerText(cs[i]);
				break;
			case 3:	//TEXT_NODE
				str += cs[i].nodeValue;
				break;
		}
	}
	return str;
}

function sortables_init() {

	// Find all tables with sortable ID and make them sortable
	var $tbls = $("table[id*='_srt_']");
	if ($tbls.length < 1) {
	tVars.tablesReady = true;
	return;
	} else {
	$tbls.each(function(){
		ts_makeSortable(this);
	});
	tVars.tablesReady = true;
	}
}


// This function looks for H&M configuration tags stored in anchors entered in H&M.
// If the tag is an alternative icon prefix it will return the icon prefix.

function hm_TagCheck(tElem,aSubstring){
			var nameTags = tElem.getElementsByTagName('a');
			var prefixFinder = /-(.*?)_/;
			var thePrefix = "";
			if (nameTags[0]) {
				for (i=0;i<nameTags.length;i++) {
					if ((nameTags[i].name.indexOf(aSubstring) != -1)  || (nameTags[i].id.indexOf(aSubstring) != -1)){

					if (aSubstring == customicons) {
							if (nameTags[i].id) {
								thePrefix = prefixFinder.exec(nameTags[i].id)[1] + "_";
								} else {
								thePrefix = prefixFinder.exec(nameTags[i].name)[1] + "_";
								}
						
						// If the image prefix is incorrect default to standard images
						// Disabled because the width trick has stopped working on current browsers
						/*var checkImg = new Image();
			            checkImg.src = image_path + thePrefix + image_up;
			            var checkWidth = checkImg.width;
			            var checkHeight = checkImg.height;
			            // Brain-dead IE returns 28 x 30 for non-existent images
			            if ((checkWidth == 0) || (checkWidth+checkHeight == 58)) {
			            	thePrefix = "";
			            	} */
						return thePrefix;
						}
					return true;
					}
				}
			} else
			return false;
	}

// This is the main routine that turns tables into sortable tables

function ts_makeSortable(t) {

	var tableID = t.id;
	var iconPrefix = "";
	var lastRow = t.rows.length-1;
	var hasSortCols = false;


	if (t.rows && t.rows.length > 0) {
		if (t.tHead && t.tHead.rows.length > 0) {
			var firstRow = t.tHead.rows[t.tHead.rows.length-1];
			thead = true;
		} else {
			var firstRow = t.rows[0];
		}
	}

	// Check for a valid header row and exit if not present
	var checkTH = t.rows[0].getElementsByTagName("th");
	var checkTH2 = t.rows[1].getElementsByTagName("th");

	if (!firstRow || !checkTH[0]) {
		alert("Error! Sortable table with ID " + tableID + " has no valid header row!\nSortable tables must have 1 and only 1 header row.");
		return;
		} else if (checkTH2[0]) {
			alert("Error! Sortable table with ID " + tableID + " has more than 1 header row!\nSortable tables must have 1 and only 1 header row.");
			return;
			} else if (t.rows.length < 4) {
			alert("Error! Sortable table with ID " + tableID + " does not have enough sortable rows!\nSortable tables must have at least three sortable rows, including the bottom row.");
			return;
			}

	// We have a header row so now we make its contents clickable links.
    
    // Set alternate rows flag to true by default
    
    altTables[tableID+"alternate"] = true;

	// Check the first row for H&M configuration anchors and apply.

		for (var i=0;i<firstRow.cells.length;i++) {
		var cell = firstRow.cells[i];
		var txt = ts_getInnerText(cell);
		var cellHTML = cell.innerHTML;
		txt = trim(txt);
		/* txt = txt.replace(/\(/,"xxleft");
		txt = txt.replace(/\)/,"xxright");
		
		cellHTML = cellHTML.replace(/\(/,"xxleft");
		cellHTML = cellHTML.replace(/\)/,"xxright"); */

   // Get the H&M Span tag for the text if present to preserve H&M header formatting
        // var attrFind = new RegExp("<span.*?>"+txt+"<\/span>","i");
        var attrFind = new RegExp("<span.*?>.*<\/span>","i");
   		var spanString = attrFind.exec(cellHTML);
 		if (spanString) {
			txt = spanString;
			}
   // Transfer paragraph margins and alignment to table cell attributes
        var cellPara = cell.getElementsByTagName("p")[0];
        var spaceBelow, spaceAbove, spaceLeft, spaceRight, paraAlign;
        var tPadding = t.cellPadding;
		if (!tPadding) tPadding = $(cell).css("padding");
		if (!tPadding) tPadding = "0px";

		if (cellPara) {
           	   var pClass = cellPara.className;

           	   // Get para style alignment attribute and apply if it exists
           	   if (pClass) paraAlign = getCssAttrib(pClass,"textAlign");
           	   // Overwrite para style alignment attribute with inline attribute if it exists
           	   paraAlign = cellPara.style.textAlign;
			   if (!paraAlign || paraAlign == "") paraAlign="left";
           	   
           	   // Set the cell alignment attribute to emulate paragraph alignment
           	   if (paraAlign.length > 0) {
           	   	   cell.align = paraAlign;

            	   }
           	   
           	   // Set the cell padding to emulate the margin settings
           	   if(cellPara.style.margin) {
           	   		spaceBelow = cellPara.style.marginBottom;
           	   		spaceAbove = cellPara.style.marginTop;
           	   		} else if (pClass && !cellPara.style.margin) {
           			spaceBelow = getCssAttrib(pClass,"marginBottom");
           			spaceAbove = getCssAttrib(pClass, "marginTop");
					spaceLeft = getCssAttrib(pClass,"marginLeft");
					spaceright = getCssAttrib(pClass,"marginRight");
           			} else {
           				spaceBelow = "0px";
           				spaceAbove = "0px";
           				spaceRight = "0px";
           				spaceLeft = "0px";
           				}
            if (!spaceBelow) spaceBelow = "0px";
          	if (!spaceAbove) spaceAbove = "0px";
            if (!spaceLeft) spaceLeft = "0px";
          	if (!spaceRight) spaceRight = "0px";
            spaceBelow = parseInt(tPadding) + parseInt(spaceBelow);
         	spaceBelow = spaceBelow + "px";

         	cell.style.paddingBottom = spaceBelow;
         	spaceAbove = parseInt(tPadding) + parseInt(spaceAbove);
         	spaceAbove = spaceAbove + "px";
          	cell.style.paddingTop = spaceAbove;
         	spaceLeft = parseInt(tPadding) + parseInt(spaceLeft);
         	spaceLeft= spaceLeft + "px";
          	cell.style.paddingLeft = spaceLeft;
         	spaceRight = parseInt(tPadding) + parseInt(spaceRight);
         	spaceRight = spaceRight + "px";
          	cell.style.paddingRight = spaceRight;
           	} // End if(cellPara)

	// Turn off alernating rows for this table if specified
		if (hm_TagCheck(cell,noalternate) && altTables[tableID+"alternate"]) {
		altTables[tableID+"alternate"] = false;
		} 
	// Get the alternating row and last row background colors and set the alternate flag
  if (altTables[tableID+"alternate"]) {
	altTables[tableID+"evenRowColor"] = t.rows[2].cells[0].style.backgroundColor;
		if (!altTables[tableID+"evenRowColor"]) {altTables[tableID+"evenRowColor"] = "transparent";}
		// alert("Table "+tableID+" even row color: "+altTables[tableID+"evenRowColor"]);
	altTables[tableID+"oddRowColor"] = t.rows[1].cells[0].style.backgroundColor;
		if (!altTables[tableID+"oddRowColor"]) {altTables[tableID+"oddRowColor"] = "transparent";}
		if (t.style.backgroundColor){altTables[tableID+"oddRowColor"] = t.style.backgroundColor; }
		// alert("Table "+tableID+" odd row color: "+altTables[tableID+"oddRowColor"]);
	altTables[tableID+"lastRowColor"] = t.rows[lastRow].cells[0].style.backgroundColor;
		if (!altTables[tableID+"lastRowColor"]) {altTables[tableID+"lastRowColor"] = "transparent";}
		// alert("Table "+tableID+" last row color: "+altTables[tableID+"lastRowColor"]);
	if (altTables[tableID+"evenRowColor"] != altTables[tableID+"oddRowColor"]) {
			altTables[tableID+"alternate"] = true; } else {altTables[tableID+"alternate"] = false;}
	} // End get alternate rows
	
	// Set individual sort icons if specified

	if (!altTables[tableID+"iconprefix"]) {
		if (hm_TagCheck(cell,customicons)) {

			altTables[tableID+"iconprefix"] = hm_TagCheck(cell,customicons);
		} else if (altTables[tableID+"iconprefix"] != "") {
			altTables[tableID+"iconprefix"] = "";
			}
		iconPrefix = altTables[tableID+"iconprefix"];
		}

	// Set sortable columns 

	if (hm_TagCheck(cell,sortablecol)) {
		cell.className = "sortablecol";
		}

	// Set fixed bottom row if specified

		var lastRow = t.rows.length-1;

		if (hm_TagCheck(cell,bottomrow)) {
			t.rows[lastRow].className = "sortbottom";
			}

    // Set default sort column if present. 

	if ((hm_TagCheck(cell,defaultsort)) || (hm_TagCheck(cell,defaultsortD))) {
		cell.className = "defaultsort";
		if (hm_TagCheck(cell,defaultsortD)) {
			cell.setAttribute("hm.descend","1");
			}
		var firstsort = i;
		}


    // Set up the header cells of the sortable table for all sortable columns

 		if ((cell.className == "sortablecol") || (cell.className.indexOf("sortablecol") != -1) || (cell.className == "defaultsort") || (cell.className.indexOf("defaultsort") != -1))  {
		    hasSortCols = true;
			if ((cell.className == "defaultsort") || (cell.className.indexOf("defaultsort") != -1)) {
			cell.innerHTML = '<a href="#" class="sortheader" id="defaultsort'+tableID+'" title="'+sort_tip+'" onclick="ts_resortTable(this, '+i+');return false;">'+txt+'<span class="sortarrow"><img src="'+ image_path + iconPrefix + image_none + '" title="'+sort_tip+'" alt="&darr;"/>&nbsp;</span></a>';
			} else {
			cell.innerHTML = '<a href="#" class="sortheader" title="'+sort_tip+'" onclick="ts_resortTable(this, '+i+');return false;">'+txt+'<span class="sortarrow">&nbsp;<img src="'+image_path+iconPrefix+image_none+'" title="'+sort_tip+'" alt="&darr;"/>&nbsp;</span></a>';
			}

		} else {
			var dummyImg = new Image();
			// Dummy image is invisible so get size of other image
			dummyImg.src = image_path + iconPrefix + image_none;
			var dummyWidth = dummyImg.width;
			var dummyHeight = dummyImg.height;
			cell.innerHTML = txt + '&nbsp;<img src="'+ image_path + iconPrefix + image_empty + '" width="' + dummyWidth +'" height="' + dummyHeight + '"/>&nbsp;&nbsp;';
			}
	} // End of first row configuration

	if (!hasSortCols) {
		alert("Error! Sortable table with ID: "+tableID+" has no sortable columns.\nSortable tables must have at least one sortable column.");
		return;
	};


// Apply alternate row colors or not depending on settings

if (altTables[tableID+"alternate"]) {alternate(t);}

   // Sort by the default column if set
     var defaultCol = document.getElementById('defaultsort' + tableID);
     if (defaultCol) {
      ts_resortTable(defaultCol,firstsort);
      if (defaultCol.parentNode.getAttribute("hm.descend") == "1") {
      	ts_resortTable(defaultCol,firstsort);}
     }
	} // End of sortable tables init routine

function ts_resortTable(lnk, clid) {
	var span;
	var sortRows = lnk.childNodes.length;
	
	for (var ci=0;ci<lnk.childNodes.length;ci++) {
		if (lnk.childNodes[ci].tagName && lnk.childNodes[ci].tagName.toLowerCase() == 'span') span = lnk.childNodes[ci];
	}
	var spantext = ts_getInnerText(span);
	var td = lnk.parentNode;
	var column = clid || td.cellIndex;
	var t = getParent(td,'TABLE');

	var tableID = t.id;
	var iconPrefix = altTables[tableID+"iconprefix"];

	// Work out a type for the column
	if (t.rows.length <= 1) return;
	var itm = "";
	var i = 1;
	while (itm == "" && i < t.tBodies[0].rows.length) {
		var itm = ts_getInnerText(t.tBodies[0].rows[i].cells[column]);
		itm = trim(itm);
		if (itm.substr(0,4) == "<!--" || itm.length == 0) {
			itm = "";
		}
		i++;
	}
	if (itm == "") return;
	sortfn = ts_sort_caseinsensitive;
	if (
		((itm.match(/^(?=\d)(?:(?:(?:(?:(?:0?[13578]|1[02])(\/|-|\.)31)\1|(?:(?:0?[1,3-9]|1[0-2])(\/|-|\.)(?:29|30)\2))(?:(?:1[6-9]|[2-9]\d)?\d{2})|(?:0?2(\/|-|\.)29\3(?:(?:(?:1[6-9]|[2-9]\d)?(?:0[48]|[2468][048]|[13579][26])|(?:(?:16|[2468][048]|[3579][26])00))))|(?:(?:0?[1-9])|(?:1[0-2]))(\/|-|\.)(?:0?[1-9]|1\d|2[0-8])\4(?:(?:1[6-9]|[2-9]\d)?\d{2}))($|\ (?=\d)))?(((0?[1-9]|1[012])(:[0-5]\d){0,2}(\ [AP]M))|([01]\d|2[0-3])(:[0-5]\d){1,2})?$/)) && (!europeandate))
		||
((itm.match(/^((((0?[1-9]|[12]\d|3[01])[\.\-\/](0?[13578]|1[02])[\.\-\/]((1[6-9]|[2-9]\d)?\d{2}))|((0?[1-9]|[12]\d|30)[\.\-\/](0?[13456789]|1[012])[\.\-\/]((1[6-9]|[2-9]\d)?\d{2}))|((0?[1-9]|1\d|2[0-8])[\.\-\/]0?2[\.\-\/]((1[6-9]|[2-9]\d)?\d{2}))|(29[\.\-\/]0?2[\.\-\/]((1[6-9]|[2-9]\d)?(0[48]|[2468][048]|[13579][26])|((16|[2468][048]|[3579][26])00)|00)))|(((0[1-9]|[12]\d|3[01])(0[13578]|1[02])((1[6-9]|[2-9]\d)?\d{2}))|((0[1-9]|[12]\d|30)(0[13456789]|1[012])((1[6-9]|[2-9]\d)?\d{2}))|((0[1-9]|1\d|2[0-8])02((1[6-9]|[2-9]\d)?\d{2}))|(2902((1[6-9]|[2-9]\d)?(0[48]|[2468][048]|[13579][26])|((16|[2468][048]|[3579][26])00)|00))))$/)) && (europeandate))
	)

	{
		sortfn = ts_sort_date;
		} else {
	var patt1 = new RegExp(/^[^\w]{0,3}([\d.,]+)[^\w]{0,3}$/);
	if (itm.match(patt1)) {
		sortfn = ts_sort_numeric;
		}
	}

	SORT_COLUMN_INDEX = column;
	var firstRow = new Array();
	var newRows = new Array();
	for (k=0;k<t.tBodies.length;k++) {
		for (i=0;i<t.tBodies[k].rows[0].length;i++) {
			firstRow[i] = t.tBodies[k].rows[0][i];
		}
	}
	for (k=0;k<t.tBodies.length;k++) {
		if (!thead) {
			// Skip the first row
			for (j=1;j<t.tBodies[k].rows.length;j++) {
				newRows[j-1] = t.tBodies[k].rows[j];
			}
		} else {
			// Do NOT skip the first row
			for (j=0;j<t.tBodies[k].rows.length;j++) {
				newRows[j] = t.tBodies[k].rows[j];
			}
		}
	}
	newRows.sort(sortfn);
	if (span.getAttribute("sortdir") == 'down') {
			ARROW = '&nbsp;<img src="'+ image_path + iconPrefix + image_down + '" title="'+sort_tip+'" alt="&darr;"/>&nbsp;';
			newRows.reverse();
			span.setAttribute('sortdir','up');
	} else {
			ARROW = '&nbsp;<img src="'+ image_path + iconPrefix + image_up + '" title="'+sort_tip+'" alt="&uarr;"/>&nbsp;';
			span.setAttribute('sortdir','down'); 
	}
	
	
    // We appendChild rows that already exist to the tbody, so it moves them rather than creating new ones
    // don't do sortbottom rows
    for (i=0; i<newRows.length; i++) {
		if (!newRows[i].className || (newRows[i].className && (newRows[i].className.indexOf('sortbottom') == -1))) {
			t.tBodies[0].appendChild(newRows[i]);
		}
	}
    // do sortbottom rows only
    for (i=0; i<newRows.length; i++) {
		if (newRows[i].className && (newRows[i].className.indexOf('sortbottom') != -1))
			t.tBodies[0].appendChild(newRows[i]);
	}
	// Delete any other arrows there may be showing
	var allspans = document.getElementsByTagName("span");
	for (var ci=0;ci<allspans.length;ci++) {
		if (allspans[ci].className == 'sortarrow') {
			if (getParent(allspans[ci],"table") == getParent(lnk,"table")) { // in the same table as us?
				allspans[ci].innerHTML = '&nbsp;<img src="'+ image_path + iconPrefix + image_none + '" title="'+sort_tip+'" alt="&Dagger;" />&nbsp;';
			}
		}
	}
	span.innerHTML = ARROW;


	// Apply alternate row colors depending on settings

	if (altTables[tableID+"alternate"]) {alternate(t);}

	}


function getParent(el, pTagName) {
	if (el == null) {
		return null;
	} else if (el.nodeType == 1 && el.tagName.toLowerCase() == pTagName.toLowerCase()) {
		return el;
	} else {
		return getParent(el.parentNode, pTagName);
	}
}

function sort_date(date) {
	// y2k notes: two digit years less than 50 are treated as 20XX, greater than 50 are treated as 19XX
	dt = "00000000";
	date = trim(date);
	date = date8(date);
		if (date.length == 8) {
		yr = date.substr(6,2);
		if (parseInt(yr) < 50) {
			yr = '20'+yr;
		} else {
			yr = '19'+yr;
		}
		} else {
			yr = date.substr(6,4);
			}
		if (europeandate == true) {
			dt = yr+date.substr(3,2)+date.substr(0,2);
			return dt;
		} else {
			dt = yr+date.substr(0,2)+date.substr(3,2);
			return dt;
		}
	
	return dt;
}

function date8(a) {
	a = a.replace(/^(\d[\.\/-])/,"0$1");
	a = a.replace(/[\.\/-](\d)(?!\d)/,"/0$1");
	return a;
	}

function ts_sort_date(a,b) {
	dt1 = sort_date(ts_getInnerText(a.cells[SORT_COLUMN_INDEX]));
	dt1 = trim(dt1);
	dt2 = sort_date(ts_getInnerText(b.cells[SORT_COLUMN_INDEX]));
	dt2 = trim(dt2);

	if (dt1==dt2) {
		return 0;
	}
	if (dt1<dt2) {
		return -1;
	}
	return 1;
}
function ts_sort_numeric(a,b) {
	var aa = ts_getInnerText(a.cells[SORT_COLUMN_INDEX]);
	aa = clean_num(aa);
	var bb = ts_getInnerText(b.cells[SORT_COLUMN_INDEX]);
	bb = clean_num(bb);
	return compare_numeric(aa,bb);
}
function compare_numeric(a,b) {
	var a = parseFloat(a);
	a = (isNaN(a) ? 0 : a);
	var b = parseFloat(b);
	b = (isNaN(b) ? 0 : b);
	return a - b;
}
function ts_sort_caseinsensitive(a,b) {
	if (!sortUmlauts) {
		aa = ts_getInnerText(a.cells[SORT_COLUMN_INDEX]).toLowerCase();
		bb = ts_getInnerText(b.cells[SORT_COLUMN_INDEX]).toLowerCase();
		} else  { 
			aa = makeSortString(encodeURIComponent(ts_getInnerText(a.cells[SORT_COLUMN_INDEX]).toLowerCase()));
			bb = makeSortString(encodeURIComponent(ts_getInnerText(b.cells[SORT_COLUMN_INDEX]).toLowerCase()));
			aa = decodeURIComponent(aa);
			bb = decodeURIComponent(bb);
			}		

	if (aa==bb) {
		return 0;
	}
	if (aa<bb) {
		return -1;
	}
	return 1;
}
function ts_sort_default(a,b) {
	if (!sortUmlauts) {
		aa = ts_getInnerText(a.cells[SORT_COLUMN_INDEX]);
		bb = ts_getInnerText(b.cells[SORT_COLUMN_INDEX]);
		} else { 
			aa = makeSortString(encodeURIComponent(ts_getInnerText(a.cells[SORT_COLUMN_INDEX])));
			bb = makeSortString(encodeURIComponent(ts_getInnerText(b.cells[SORT_COLUMN_INDEX])));
			aa = decodeURIComponent(aa);
			bb = decodeURIComponent(bb);
			}
	if (aa==bb) {
		return 0;
	}
	if (aa<bb) {
		return -1;
	}
	return 1;
}
function clean_num(str) {
	var repPattern = new RegExp(/[^-?0-9.]/g);
	if (germanNumbers) repPattern = new RegExp(/[^-?0-9,]/g);
	str = str.replace(repPattern,"");
	return str;
}
function alternate(table) {
	var tableID = table.id;
	// Take object table and get all its tbodies.
	var tableBodies = table.getElementsByTagName("tbody");

	// Remove the H&M background colors
	var tableCells = table.getElementsByTagName("td");
	for (var i = 0; i < tableCells.length; i++) {
		tableCells[i].style.backgroundColor = "";
		}

	// Loop through these tbodies
	for (var i = 0; i < tableBodies.length; i++) {
		// Take the tbody, and get all its rows
		var tableRows = tableBodies[i].getElementsByTagName("tr");
		// Loop through these rows
		// Start at 1 because we want to leave the heading row untouched
		for (var j = 1; j < tableRows.length; j++) {
			// Check if j is even, and apply classes for both possible results
			if ( ((j % 2) == 0) ) {
				if ( !(tableRows[j].className.indexOf('odd') == -1) ) {
					tableRows[j].className = tableRows[j].className.replace('odd', 'even');
				} else {
					if ( tableRows[j].className.indexOf('even') == -1 ) {
						tableRows[j].className += " even";
					}
				}
			} else {

				if ( !(tableRows[j].className.indexOf('even') == -1) ) {
					tableRows[j].className = tableRows[j].className.replace('even', 'odd');
				} else {
					if ( tableRows[j].className.indexOf('odd') == -1 ) {
						tableRows[j].className += " odd";
					}
				}
			}

		}
	}

	// Apply the H&M alternating background colors
	var newColor;
	for (var i = 1; i < tableRows.length; i++) {
			
			// Remove HTML background attribs from non-XHTML version
			for (var j = 0; j < tableRows[i].cells.length; j++) {
			tableRows[i].cells[j].removeAttribute("bgColor");
			}

		if (tableRows[i].className.indexOf("even") != -1) {
			newColor = altTables[tableID+"evenRowColor"];
			}
		if (tableRows[i].className.indexOf("odd")!= -1) {
			newColor = altTables[tableID+"oddRowColor"];
			}
		if (tableRows[i].className.indexOf("sortbottom") != -1) {
			newColor = altTables[tableID+"lastRowColor"];}
		tableRows[i].style.backgroundColor = newColor;

		} // End apply alternating colors

}
var sortableLoaded = true;
