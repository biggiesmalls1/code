// ==UserScript==
// @name           Title Youtube Locations
// @namespace      TYTLs
// @description    Forces all YouTube pages to put the video title in the Location Bar
// @include        http://*.youtube.*/*
// @include        http://youtube.*/*
// ==/UserScript==

// TODO: In case for some reason YouTube or another script redirects us to
// another URL which has lost the title param we added, we will re-run forever.
// To avoid this we should have a fallback.  E.g. if the title we want to set
// is the same as the last one we did set (via GM_set/getValue) then do not try
// again.

function getCGIParams() {
	var toParse = document.location.search.replace(/^\?[&]*/,'');
	var params = new Object();
	while (toParse.length) {
		var key = toParse.match(/^[^=]*/);
		toParse = toParse.replace(/^[^=]*=/,'');
		var value = toParse.match(/^[^&]*/);
		toParse = toParse.replace(/^[^&]*[&]*/,'');
		params[key] = decodeURIComponent(value);
	}
	return params;
}

setTimeout(function(){
	if (document.location.pathname == "/watch") {

		var title = document.getElementsByTagName("h1")[0].textContent
			|| document.title.replace(/^YouTube - /,'')
			|| null;

		if (title)
			title = title.replace(/ /g,'_').replace(/^[\r\n_]*/,'').replace(/[\r\n_]*$/,''); // "_"s paste better into IRC, since " "s become "%20"s which are hard to read.  The second and third parts trim "_"s and newlines from the start and end of the string.

		if (title) {
			if (!document.location.hash) {
				document.location.replace(document.location.href + '#' + title); // Does not alter browser history
				// document.location.hash = title; // Crashes Chrome less often
			}
		}
	}
},5000); // This is what really stops the crashing!



// == Scrollbars on comments and related vids, to keep the video in view. ==
setTimeout(function(){
	// We could alternatively act on watch-panel but that includes the video navigation buttons!
	// BUG: YouTube doesn't load the later thumbnails until we scroll the actual page.  :S
	var watchDiscussion = document.getElementById("watch-discussion");
	if (watchDiscussion) {
		watchDiscussion.style.overflow = "auto";
		watchDiscussion.style.maxHeight = (window.innerHeight - 552)+"px"; /* For a video height 360p */
	}
	var watchSidebar = document.getElementById("watch-sidebar");
	if (watchSidebar) {
		watchSidebar.style.overflow = "auto";
		watchSidebar.style.maxHeight = (window.innerHeight - 61)+"px";
		// Now the text wraps because of the scrollbar, so we widen the element:
		watchSidebar.style.width = (320+24)+"px";
		// And we must widen its container also:
		// TODO BUG: Why does this work in the console, but not from the userscript?
		document.getElementById("watch-main").style.width = (960+24)+"px";
	}
	// Title text
	//document.getElementById("eow-title").scrollIntoView();
	// Uploader info and videolist popdown.
	//document.getElementById("watch-headline-user-info").scrollIntoView();
	// The video (was supposed to be a small gap above the video but failed)
	document.getElementById("watch-more-from-user").scrollIntoView();
	// The video
	//document.getElementsByTagName("embed")[0].scrollIntoView();
	// The video
	//document.getElementById("watch-video").scrollIntoView();
},1000);



// == Thumbnail animation ==
// TODO: This is working fine on "related videos" thumbnails, but not on queue
// thumbnails, even if I have the queue open when I load the page.
// Perhaps we are responding to a mouseout event from a child element, because
// we are not checking the event target like we should do.
function initThumbnailAnimator() {
	// function createThumbnailAnimatorEvent(img) {
	var img    = null;
	var evt    = null;
	var timer  = null;
	var frames = ["1.jpg","2.jpg","3.jpg"];   // "default.jpg",
	var frameI = 0;
	function changeFrame() {
		frameI = (frameI + 1) % frames.length;
		img.src = img.src.replace(/\/[^/]*$/,'') + '/' + frames[frameI];
	}
	function startAnimation() {
		// We make this check quite late, due to lazy loading
		if (img.src.match(/default\.jpg$/)) {
			timer = setInterval(changeFrame,600);
		}
	}
	function stopAnimation() {
		if (timer) {
			logElem("Stopping elem",img);
			clearInterval(timer);
			// This isn't really neccessary, except to ensure the check for default\.jpg above works next time!
			img.src = img.src.replace(/\/[^/]*$/,'') + '/' + "default.jpg";
		}
	}
	function logElem(name,elem) {
		report = "<"+elem.tagName+" id="+elem.id+" class="+elem.className+" src="+elem.src+" />";
		GM_log(name+" = "+report);
	}
	function check(fn) {
		return function(e) {
			evt = e;
			// logElem("["+evt.type+"] evt.target",evt.target);
			var elemToCheck = evt.target || evt.srcElement;
			var imgCount = elemToCheck.getElementsByTagName("img").length;
			if (elemToCheck.tagName == "IMG") {
				img = event.target;
				return fn();
			// } else if (imgCount == 1) {
				// img = elemToCheck.getElementsByTagName("img")[0];
				// // logElem("["+evt.type+"] checking sub-image",img);
				// logElem("Whilst checking",elemToCheck);
				// logElem("  Animating elem",img);
				// logElem("  with parent",img.parentNode);
				// logElem("  whilst currentTarget",evt.currentTarget);
				// logElem("  and srcElement",evt.srcElement);
				// return fn();
			} else if (elemToCheck.className=='screen') {
				var seekImg = elemToCheck.parentNode.getElementsByTagName("img")[0];
				if (seekImg) {
					img = seekImg;
					fn();
				}
			}
		};
	}
	//// Unfortunately these do not fire on any HTMLImageElements when browsing the queue.
	document.body.addEventListener("mouseover",check(startAnimation),false);
	document.body.addEventListener("mouseout",check(stopAnimation),false);
	// var videoList = document.getElementById("watch-sidebar"); // or watch-module or watch-module-body or watch-related or watch-more-related
	// var videoList = document.getElementsByClassName("video-list")[0]; // can be 4 of these!
	// var thumbs = document.getElementsByTagName("img");
	// for (var i=0;i<thumbs.length;i++) {
		// createThumbnailAnimatorEvent(thumbs[i]);
	// }
}
setTimeout(initThumbnailAnimator,1000);

