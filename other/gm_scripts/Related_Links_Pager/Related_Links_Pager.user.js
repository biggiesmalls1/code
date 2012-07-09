// ==UserScript==
// @name           Related Links Pager
// @namespace      RLP
// @description    Navigate sideways!  When you click a link, related links on the current page are carried with you.  They can be accessed from a pager on the target page, so you won't have to go back in your browser.
// @include        http://*/*
// @include        https://*/*
// @exclude        http://www.facebook.com/*
// @exclude        https://www.facebook.com/*
// ==/UserScript==



// == OPTIONS ==

var delayBeforeRunning = 2000;
var minimumGroupSize = 2;
var maximumGroupSize = 250;           // TODO: should really be based on length of final URL, which some webservers restrict ("Bad Request").  I've seen Chrome on Wikipedia manage 200 siblings!
var groupLinksByClass = true;

var enableOnCtrlClick = true;
var enableOnShiftClick = true;       // Allows you to avoid the script when needed
var enableOnRightClick = false;

var keepNavigationHistory = false;    // When off, paging is not added to browser history.  The back button will return you to the page before you started paging.
var leaveHashUrlsAlone = true;        // Many sites use # these days for their own purposes - this avoids the risk of breaking them.
var forceTravel = false;              // Attempt to fix loss of #data when clicking thumbnails on YouTube.  Failed to fix it!
                                      // BUG: I think this overrides Google Preview Pane's handler if we click a link expecting it to be previewed.
var clearDataFromLocation = false;    // Looks tidier but can prevent Pager from re-appearing when navigating Back to this page (or reloading it) - OR adds an extra step to history, depending on the implementation chosen below.

var verbose = false;                  // Extra logging for debugging

var highlightLinkGroups = true;       // Change the background color of links in the current group on hover.
// var thisLinkHighlightColor = "rgba(130,200,255,0.1)"; // light blue
// var highlightColor         = "rgba(130,200,255,0.2)"; // very light blue
// var thisLinkHighlightColor = "rgba(130,230,255,0.3)";
// var highlightColor         = null;
var thisLinkHighlightColor = "rgba(0,0,255,0.0)";
var highlightColor         = "rgba(0,0,255,0.8)"; // very light blue

var showGroupCountInLinkTitle = true; // Updates hovered links' titles to show number of siblings.
var showPageNumberInWindowTitle = true;



// == CHANGELOG ==
// 2012-03-22 Added highlighting and further heuristics.
// 2012-02-07 Bugfixes and more heuristics.
// 2012-01-30 Fixed repeat rewrite bug by checking for &sib as well as #sib.
// 2012-01-28 Fixed close button positioning in Firefox.
// 2012-01-28 Restricted related links to those with the same CSS class.
//       BUG: We still group unrelated (indistinguishable) links on Wikipedia!
// 2012-01-28 Blacklisted some buggy situations.



// == NOTES ==

// You can avoid this script either by clicking a link before it runs or by
// activating the desired link with the keyboard instead of the mouse.

// TODO: The # method seems to work but could present a BUG on some sites.  It
// is sometimes needed but it's horrible.  Use alternatives where possible.  In
// Chrome that could be localStorage *if* the followed sibling is on the same
// domain.  In Firefox Greasemonkey we can use GM_set/getValue().

// TODO: Pager can only go sideways.  It could also offer ability to go "up"
// to the page that contained all the links in the current group.

// BUG: Does not work through redirects.

// CONSIDER: Would it be better to use a normal CGI '&' instead of '#' ?
// Should we compress the data to survive through more webservers?

// TODO: Do not load pager if it is redundant (i.e. if we can already see the
// sibling links on the current page!).

// DONE: removeRedirection only runs on links originally in the page, not
// those added later, e.g. by Ajax when refining a Google search.  This may be
// the cause of some issues.  (Google says "This url is too long.")

// FIXED: Now we are passing ,1 to mark the "current" page, but this is
// being passed into the siblings package.  The package needs to be rebuilt and
// altered!  That was a fairly rare case: redirected to a new domain but
// siblings retained!

// BUG TODO: With forceTravel=true, submitting an answer on StackOverflow, we
// get a warning message that we are about to leave the current page!  (In fact
// is was fine for me to accept the warning and the post happened ok, but still
// hardly tidy!)
//
// Recommend: Default to forceTravel=false, and override with heuristics only
// for Google search!  So it works gently for all users, never causing
// problems, but sometimes failing(being invisibly overriden - gah!).  Enable
// it for personal use, as other powerusers might do.
//
// Another place our script is overzealous is on StackOverflow, when clicking
// to expand a post, AJAX works just fine, but forceTravel sends us to a
// different page regardless!  Solution: don't click on the link, click on the
// div.
//
// I can't see any way how forceTravel could detect whether the user wants it
// or not.  Well, the user wants it only if clicking would take them to a new
// page.  Could we perhaps catch the page change event and if it happened
// immediately after a click, ensure the target URL has siblings packet.  I'm
// not sure JS has the power to do that through onbeforeunload.
//
// Perhaps we should go for a medium option - remove .onclick and unregister
// any event handlers (we can't do that!).  Sites *should* fall back to their
// non-JS method.
//
// Oh well we already are removing onmousedown, perhaps we should do click and
// mouseup also, but make them all optional.

// TODO: Appending # data breaks Twitter.  @exclude is not the solution.  We
// should just not append to links who land on twitter (from in or out), whilst
// links leaving Twitter should be fine!



// We grab the data as early as possible, in case any other scripts decide to
// change the #.  We delay running anything else for a little while.
var grabbedList;
if (document.location.hash && document.location.hash.indexOf("siblings=")>=0) {
  grabbedList = document.location.hash.replace(/.*[#&]siblings=([^#]*)/,'$1');
}



function runRelatedLinksPager(){



// Library functions

function getXPath(node) {
	var parent = node.parentNode;
	if (!parent) {
		return '';
	}
	var siblings = parent.childNodes;
	var totalCount = 0;
	var thisCount = -1;
	for (var i=0;i<siblings.length;i++) {
		var sibling = siblings[i];
		if (sibling.nodeType == node.nodeType) {
			totalCount++;
		}
		if (sibling == node) {
			thisCount = totalCount;
			break;
		}
	}
	return getXPath(parent) + '/' + node.nodeName.toLowerCase() + (totalCount>1 ? '[' + thisCount + ']' : '' );
}

// What can I say?  I loooove favicons!
// BUG: Does not add a favicon for the current page, because the current page
// is not shown as a link.  This breaks left-alignment of the text!
function addFaviconToLinkObviouslyIMeanWhyWouldntYou(link) {

  if (!link.href) {
    return;
  }

  var host = link.href.replace(/^[^\/]*:\/\//,'').replace(/\/.*$/,'');
  var img = document.createElement('IMG');
  // img.src = 'http://'+host+'/favicon.ico';

  var alwaysUseGoogle = false;
  var imageExtensions = ( alwaysUseGoogle ? [] : ['gif','jpg','png','ico'] );
  function tryExtension(evt) {
    var ext = imageExtensions.pop();
    if (ext) {
      img.src = 'http://'+host+'/favicon.'+ext;
    } else {
      img.title = "Failed to find favicon for "+host;
      img.src = 'http://www.google.com/s2/favicons?domain=' + host; // Google's cache will sometimes provide a favicon we would have missed, e.g. if the site uses .png instead of .ico.  Thanks to NV for suggesting this, and to Google.
      // @consider We could also generate an md5sum and request a gravatar, which might simply allow human recognition of repeats.
      img.removeEventListener('error',tryExtension,true);
    }
  }
  img.addEventListener('error',tryExtension,true);
  tryExtension();

  img.title = ''+host;
  img.style.border = '0';
  img.style.paddingRight = '4px';
  img.style.width = '1.0em';
  img.style.height = '1.0em';
  // Favicon image elements can be hidden until they have fully loaded
  // img.style.display = 'none';
  img.addEventListener('load',function(){ img.style.display = ''; },false);

  link.parentNode.insertBefore(img,link);
}

if (!this.GM_addStyle) {
  this.GM_addStyle = function(css) {
    var s = document.createElement("style");
    s.type = 'text/css';
    s.innerHTML = css;
    document.getElementsByTagName("head")[0].appendChild(s);
  };
}



// We consider related links, or "siblings", to be those on the current page
// with the same DOM path as the clicked link.

function getGroupSignature(link) {
	if (!link.cachedGroupSignature) {
		// We remove offsets like [4] from the unique xpath to get a more general path signature.
		link.cachedGroupSignature = getXPath(link).replace(/\[[0-9]*\]/g,'');
	}
	return link.cachedGroupSignature;
}

function collectLinksInSameGroupAs(clickedLink) {
  // We remove the numbers from the XPath
  var seekXPath = getGroupSignature(clickedLink);
  // NOTE: We could search for matches with document.query - it might be faster.
  var links = document.getElementsByTagName("A");
  var collected = [];
  for (var i=0;i<links.length;i++) {
    var link = links[i];
    if (groupLinksByClass && link.className != clickedLink.className) {
      continue;
    }
    var xpath = getGroupSignature(link);
    if (xpath == seekXPath) {
      if (link.textContent) {   // ignore if no title
        collected.push(link);
      }
    }
  }
  if (verbose) {
    GM_log("Got "+list.length+" matching siblings: for "+link.outerHTML+" with xpath "+seekXPath);
  }
  return collected;
}

// Collect siblings when the user clicks a link, and pass them forward to the
// target page in a hash package.

function isSuitable(link) {

  if (link.tagName != "A") {
    return;
  }

  if (link.href.indexOf("#siblings=")>=0 || link.href.indexOf("&siblings=")>=0) {
    // This is already a prepared link!  Probably created by the pager.
    // No need to modify it.
    return;
  }
  if (link.protocol.indexOf(/*"http") != 0)*/ "javascript:") == 0) {
    // We should not add #s to javascript: links but it seems to work ok on ftp:// (FF)
    return;
  }

  // Ignore links which are simply anchor into the current page
  // Note that .href gives the whole URL, so we check getAttribute("href")
  if (link.getAttribute("href") && link.getAttribute("href").charAt(0) == '#') {
    return;
  }

  // What about links to #s in other pages?  I decided in the end to leave them
  // alone by default (preserve the existing hash string).
  // Altering # strings can break the way some sites use # strings, and can
  // prevent the browser from scrolling to the anchor.
  // You can force appending of siblings package to hash strings using '&' if
  // desired by disabling leaveHashUrlsAlone.
  // Perhaps in these "emergency" circumstances, we should append with '&' or
  // '?' *outside* the hash.  (Which will no doubt break some sites, but
  // perhaps fewer!)
  if (link.hash && leaveHashUrlsAlone) {
    return;
  }

  //// === Some sites complain about long URLs or unexpected strings. ===

  // Some Google search pages complain about our long URLs.
  var googleWillComplain =
    link.host.indexOf("google")>=0 &&
      (    link.href.indexOf("?q=")>=0
        || link.href.indexOf("&q=")>=0
        || link.href.indexOf("url?")>=0
      );
  // TODO: There are more of these cases on Google!  (When earlier rewriting failed?)

  var isYouTubePagerLink =
    link.host.indexOf("www.youtube.") == 0
    && link.href.indexOf("/all_comments?") >= 0;
  var youtubeWillComplain = isYouTubePagerLink;

  var siteWillComplain = googleWillComplain || youtubeWillComplain;

  if (siteWillComplain) {
    return;
  }

  return true;

}

function checkClick(evt) {
  // var elem = evt.target || evt.sourceElement;
  var elem = seekLinkInAncestry(evt.target || evt.sourceElement);
  // GM_log("Intercepted click event on "+getXPath(elem));

  // Do not interfere with Ctrl-click or Shift-click or right-click (usually open-in-new-window/tab)
  if ((evt.ctrlKey && !enableOnCtrlClick) || (evt.shiftKey && !enableOnShiftClick) || (evt.button>0 && !enableOnRightClick)) {
    return;
  }

  if (elem.tagName == "A") {
    var link = elem;

    if (!isSuitable(link)) {
      return;
    }

    // GM_log("User clicked on link: "+link.href);
    // Collect other links matching this one:
    var linksInGroup = collectLinksInSameGroupAs(link);
    // Convert from links to records:
    var siblings = linksInGroup.map(function(l) {
      var record = [l.textContent, l.href];
      if (l.href == link.href) {
        record[2] = 1; // Mark this record as the (soon-to-be) current one
      }
      return record;
    });
    if (siblings.length <= minimumGroupSize) {
      // No point.  Give the user a clean location bar for a change.  ;)
      return;
    }
    if (siblings.length > maximumGroupSize) {
      // It would be dangerous to proceed!
      return;
    }

    // GM_log("Found "+siblings.length+" siblings for the clicked link.");
    var sibsEncoded = encodeURIComponent(JSON.stringify(siblings));
    // If the link already had a #, we append our data as an & parameter, and cross our fingers.
    var appendChar = ( link.href.indexOf('#')>=0 ? '&' : '#' );
    if (appendChar == '&' || link.hash) {
      if (verbose) {
        GM_log("Appending to existing hash with "+appendChar+": "+link.hash);
      }
      // Note: If it was a normal # to an anchor then we have probably broken
      // it!  In that case we should either not append, or perhaps we can
      // append, but force movement to the correct anchor anyway (which may be
      // on the current page, or after navigation!).
    }
    var targetURL = link.href + appendChar + "siblings="+sibsEncoded;

    if (verbose) {
      GM_log("Rewriting link "+getXPath(link));
      GM_log(" url: "+link.href);
      GM_log("with: "+targetURL);
    }

    // Force travel to the new URL.  (Don't wait for the page to handle the
    // click - some sites e.g. YouTube will throw away our hash-data!)
    if (forceTravel) {
      // We only do this for normal left-clicks.
      if (!evt.ctrlKey && !evt.shiftKey && evt.button==0) {
        document.location = targetURL;
        evt.preventDefault();
        return false;
      }
    }

    // Instead of pushing the browser to the magic URL, just change the link and see what happens.
    link.href = targetURL;

    // We need this on Google search result pages, or we end up following
    // feedback/tracking redirection links, which throw away our hash data!
    link.removeAttribute('onmousedown');
    // Thanks to http://userscripts.org/scripts/review/57679

  }
}

document.body.addEventListener("click",checkClick,true);
document.body.addEventListener("mousedown",checkClick,true);
document.body.addEventListener("mouseup",checkClick,true);



// If we have been passed a hash package of siblings, present the lovely pager.

function createRelatedLinksPager(siblings) {

  //// Find currentIndex.
  var hashPart = new RegExp("#.*");
  var seekURL = document.location.href.replace(hashPart,'');
  // var currentIndex = siblings.indexOf(seekURL);   // No because the list contains records not urls!
  var currentIndex = -1;
  for (var i=0;i<siblings.length;i++) {
    var record = siblings[i];
    /*
    //// KNOWN BUG: This can fail if the receiving website redirects us, e.g. blogspot.com pushes me to the same page on blogspot.co.uk.
    //// Poor solution: Use wordex to find closest match.
    //// Good solution: TODO: Pass forward index along with siblings, just in case.
    if (record[1].replace(hashPart,'') == seekURL) {
    */
    if (record[2]) {
      currentIndex = i;
      break;
    }
  }
  // GM_log("Current index: "+currentIndex);
  if (currentIndex == -1) {
    // This should be unlikely to happen!
    GM_log("Odd, I could not find: "+seekURL+" in the siblings list.");
    // But it does happen occasionally.
    // One time by navigating to Wikipedia's main page by clicking the top-left logo.
    // Don't return.  Show the list anyway!
    // return;
  }

  if (showPageNumberInWindowTitle) {
    document.title = document.title + " (Page "+(currentIndex+1)+" of "+siblings.length+")";
  }

  var pager = document.createElement("div");
  pager.id = "linkGroupPager";
  GM_addStyle("#linkGroupPager { position: fixed; top: 5%; right: 5%; "+
    "z-index: 9999999; background: white; color: black; border: 1px solid black; "+
    "padding: 5px; font-size: 100%; text-align: center; "+
    "max-width: 40%; max-height: 90%; overflow: auto; } "+
    ".linkGroupPagerList { text-align: left; }"
  );

  function maybeHost(link) {
    if (link.host != document.location.host) {
      return "("+link.host+")";
    } else {
      return "";
    }
  }

  function createLinkFromRecord(selectedRecord, text) {
    var link = document.createElement("A");
    link.textContent = text;
    var appendChar = ( selectedRecord[1].indexOf('#')>=0 ? '&' : '#' );

    // Move the "current page marker" to the newly selected page
    var records = siblings;
    var newRecords = records.map(function(record) {
        record = record.slice(0); // clone to preserve original
        if (record[2]) {
          record.pop();
        }
        if (record[1] === selectedRecord[1]) {
          record[2] = 1;
        }
        return record;
    });
    var newEncodedList = encodeURIComponent(JSON.stringify(newRecords));

    link.href = selectedRecord[1] + appendChar + 'siblings=' + newEncodedList;
    if (text != selectedRecord[0]) {
      link.title = selectedRecord[0];
    }
    link.title = (link.title ? link.title+' ' : '') + maybeHost(link);
    link.onclick = function(evt){
      if (!keepNavigationHistory) {
        document.location.replace(this.href);
        evt.preventDefault();
      }
    };
    return link;
  }

  if (currentIndex > 0) {
    var leftRecord = siblings[currentIndex-1];
    var leftLink = createLinkFromRecord(leftRecord, "<<");
    leftLink.title = "Previous: "+leftLink.title;
    pager.appendChild(leftLink);
  }

  // var pagerButton = document.createTextNode(" Pager ");
  var pagerButton = document.createElement("span");
  // pagerButton.textContent = " Pager ";
  pagerButton.textContent = " Page "+(currentIndex+1)+" of "+siblings.length+" ";
  pagerButton.addEventListener("click",function(evt) {
    pageList.style.display = ( pageList.style.display == 'none' ? '' : 'none' );
  },false);
  pagerButton.style.cursor = 'pointer';
  pager.appendChild(pagerButton);

  if (currentIndex < siblings.length-1) {
    var rightRecord = siblings[currentIndex+1];
    var rightLink = createLinkFromRecord(rightRecord, ">>");
    rightLink.title = "Next: "+rightLink.title;
    pager.appendChild(rightLink);
  }

  var closeButton = document.createElement("span");
  closeButton.textContent = "[X]";
  closeButton.style.cursor = 'pointer';
  closeButton.style.paddingLeft = '5px';
  closeButton.onclick = function() { pager.parentNode.removeChild(pager); };
  pager.appendChild(closeButton);

  // We could create this lazily, but why not immediately? :P
  var pageList = document.createElement("div");
  pageList.className = "linkGroupPagerList";
  for (var i=0;i<siblings.length;i++) {
    pageList.appendChild(document.createElement("br"));
    pageList.appendChild(document.createTextNode(""+(i+1)+". "));
    var record = siblings[i];
    var text = record[0] || record[1];   // use address if no title
    var link = createLinkFromRecord(record, text);
    // if (record[1] == seekURL) {
    // if (record[1].replace(hashPart,'') == seekURL) {
    if (record[2]) {
      // Replace link with just a bold span
      var span = document.createElement("span");
      span.style.fontWeight = 'bold';
      span.textContent = link.textContent;
      link = span;
    }
    pageList.appendChild(link);
    addFaviconToLinkObviouslyIMeanWhyWouldntYou(link);
  }
  pageList.style.display = 'none';
  pager.appendChild(pageList);

  // GM_log("Created pager: "+pager);
  document.body.appendChild(pager);
}

if (grabbedList) {
  var siblings = JSON.parse(decodeURIComponent(grabbedList));
  createRelatedLinksPager(siblings);
  if (clearDataFromLocation) {
    // document.location.hash = ".";    // BAD.  "#." breaks google search results pages, tho we rarely page through them.
    document.location.hash = '';        // Creates an extra history step, but the user may want that, to retain the data!
    // document.location.replace('#');  // Does not create history.  Data lost!  Fine if only navigating forwards.
  }
}



// Optional: Show link's siblings on hover

if (highlightLinkGroups) {

  function seekLinkInAncestry(startElem) {
    var node = startElem;
    while (node) {
      if (node.tagName == "A") {
        return node;
      }
      node = node.parentNode;
    }
    return startElem;
  }

  var list = [];

  function highlightList() {
    if (verbose) {
      GM_log("Highlighting "+list.length+" elements.");
    }
    for (var i=0;i<list.length;i++) {
      if (highlightColor) {
        /*
        list[i].oldBackgroundColor = list[i].style.backgroundColor;
        list[i].style.backgroundColor = highlightColor;
        */
        list[i].oldBorderLeft = list[i].style.borderLeft;
        list[i].oldMarginLeft = list[i].style.marginLeft;
        list[i].oldBorderRight = list[i].style.borderRight;
        list[i].oldMarginRight = list[i].style.marginRight;
        list[i].oldBorderTop = list[i].style.borderTop;
        list[i].oldMarginTop = list[i].style.marginTop;
        list[i].oldBorderBottom = list[i].style.borderBottom;
        list[i].oldMarginBottom = list[i].style.marginBottom;
        list[i].style.border = "1px dashed "+highlightColor;
        list[i].style.margin = "-1px";
      }
    }
  }

  function clearList() {
    for (var i=0;i<list.length;i++) {
      /*
      list[i].style.backgroundColor = list[i].oldBackgroundColor;
      */
      list[i].style.marginLeft = list[i].oldMarginLeft;
      list[i].style.borderLeft = list[i].oldBorderLeft;
      list[i].style.marginRight = list[i].oldMarginRight;
      list[i].style.borderRight = list[i].oldBorderRight;
      list[i].style.marginTop = list[i].oldMarginTop;
      list[i].style.borderTop = list[i].oldBorderTop;
      list[i].style.marginBottom = list[i].oldMarginBottom;
      list[i].style.borderBottom = list[i].oldBorderBottom;
    }
    list.length = 0;
  }

  document.body.addEventListener("mouseover",function(evt) {
    var link = seekLinkInAncestry(evt.target || evt.sourceElement);
    if (isSuitable(link)) {
      clearList();
      list = collectLinksInSameGroupAs(link);
      if (showGroupCountInLinkTitle && !link.doneAppendGroupsize) {
        link.doneAppendGroupsize = true;
        link.title = (link.title ? link.title+" " : "") + "("+list.length+" related links)";
      }
      if (list.length>=minimumGroupSize && list.length<maximumGroupSize) {
        highlightList();
        if (thisLinkHighlightColor) {
          // link.style.backgroundColor = thisLinkHighlightColor;
          link.style.border = "1px solid "+thisLinkHighlightColor;
        }
      }
    }
  },true);
  document.body.addEventListener("mouseout",function(evt) {
    var link = seekLinkInAncestry(evt.target || evt.sourceElement);
    if (isSuitable(link)) {
      clearList();
    }
  },true);

}



}



setTimeout(runRelatedLinksPager, delayBeforeRunning);

