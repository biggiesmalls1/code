// ==UserScript==
// @name           Github Notifications Dropdown
// @namespace      joeytwiddle
// @copyright      2014, Paul "Joey" Clark (http://neuralyte.org/~joey)
// @version        0.8.0
// @description    When clicking the notifications icon, displays notifications in a dropdown pane, without leaving the current page.  (Now also makes files in diff views collapsable.)
// @include        https://github.com/*
// @grant          none
// ==/UserScript==

// bug: If the notifications list is longer than the page, scroll down to the bottom and then try to click on the white space below the Github document's content.  The event does not fire there!

var mainNotificationsPath = "/notifications";

var notificationButtonLink = $(".header a.notification-indicator[href]");
var notificationButtonContainer = notificationButtonLink.parent();
var closeClickTargets = $("body, .header a.notification-indicator[href]");

var notificationsDropdown = null;
var tabArrow = null;

function listenForNotificationClick(){
	notificationButtonContainer.on("click", onNotificationButtonClicked);
}

function onNotificationButtonClicked(evt){
	// Act normally, do nothing, if modifier key is pressed.
	if (evt.ctrlKey || evt.shiftKey || evt.metaKey) {
		return;
	}
	evt.preventDefault();
	notificationButtonContainer.off("click", onNotificationButtonClicked);
	var targetPage = notificationButtonLink.attr('href');
	fetchNotifications(targetPage);
}

function fetchNotifications(targetPage){
	notificationButtonContainer.css({
		"opacity": "0.3",
		"background-color": "#ececec",
		"background-image": "linear-gradient(#d9d9d9, #ececec)"
	});
	$.ajax({
		url: targetPage,
		dataType: "html"
	}).then(receiveNotificationsPage.bind(null,targetPage)).fail(receiveNotificationsPage);
}

function receiveNotificationsPage(targetPage, data, textStatus, jqXHR){
	notificationButtonContainer.css("opacity", "");

	notificationsDropdown = $("<div>").addClass("notifications-dropdown");

	var title = "Notifications";
	var extra = null;
	if (targetPage != mainNotificationsPath) {
		title += " for " + targetPage.replace(/^\/+|\/notifications$/g,'');
	}
	var titleElem = $('<h3>').text(title);
	if (targetPage != mainNotificationsPath) {
		var buttonToSeeAll = $('<a href="#">').text('See all');
		buttonToSeeAll.on('click', function(evt){
			evt.preventDefault();
			closeNotificationsDropdown();
			fetchNotifications(mainNotificationsPath);
		});
		titleElem.append( textNode(" ("), buttonToSeeAll, textNode(")") );
	}
	notificationsDropdown.append( $("<center>").append(titleElem) );

	var notificationPage = $("<div>").append( $.parseHTML(data) );
	var notificationsList = notificationPage.find(".notifications-list");
	// Provide hover text for all links, so if the text is too long to display, it can at least be seen on hover.
	notificationsList.find("a").each(function(){
		$(this).attr("title", $(this).text().trim());
	});
	var minWidth = Math.min(750, window.innerWidth-48);
	if (notificationsList.children().length == 0) {
		notificationsDropdown.append("<center>No new notifications</center>");
		minWidth = 0;
	}
	notificationsDropdown.append(notificationsList);
	var linkToPage = mainNotificationsPath;
	//var linkToPage = targetPage;
	var seeAll = $("<center><b><a href='"+encodeURI(linkToPage)+"'>Notifications page</a></b></center>");
	notificationsDropdown.append(seeAll);

	var arrowSize = 10;

	$("<style>").html(""
	  + " .notifications-dropdown { "
	  + "   border: 1px solid #ddd; "
	  + "   background-color: #fff; "
	  + "   padding: 2px 16px; "
	  + "   box-shadow: 0px 2px 8px 0px rgba(0,0,0,0.25); "
	  + "   border-radius: 24px; "
	  //+ "   max-height: 90%; "
	  + "   margin-bottom: 20px; "   // If the body is shorter than the dropdown, the body will expand to let it fit, but only just.  This will ensure a little bit of extra space is available for the shadow and a small gap.
	  + "   z-index: 10000000; "     // To appear above the .bootcamp .desc on the front page and .table-list-header on .../issues
	  + " } "
	  + " .notifications-dropdown > center { "
	  + "   padding: 8px 8px; "
	  + " } "
	  // GitHub uses default 20px here, but it applies to the last one too, which messes up our layout.
	  + " .notifications-dropdown .notifications-list .boxed-group:not(:last-child) { "
	  + "   margin-bottom: 16px; "
	  + " } "
	  + " .notifications-dropdown .notifications-list .boxed-group:last-child { "
	  + "   margin-bottom: 0px; "
	  + " } "
	  + " .notifications-dropdown .notifications-list { "
	  + "   float: initial; "
	  + " } "
	  // No longer an issue:
	  // There was a rule on the user profile page that applies to the notification ticks (which are usually never seen on that page).  The rule matches `body.page-profile .box-header .tooltipped`.
	  // That rule messes up the position of each tick icon relative to its containing header.  So we override to the previous values.
	  /*
	  + " .notifications-dropdown .box-header .mark-all-as-read { "
	  + "   top: auto !important; "
	  + "   left: auto !important; "
	  + "   right: auto !important; "
	  + "   bottom: auto !important; "
	  + "   float: right; "
	  + " } "
	  */
	  + " .notifications-dropdown-arrow { "
	  + "   position: absolute; "
	  + "   width: 0px; "
	  + "   height: 0px; "
	  + "   border-left: "+arrowSize+"px solid transparent; "
	  + "   border-right: "+arrowSize+"px solid transparent; "
	  + "   border-bottom: "+arrowSize+"px solid white; "
	  + "   z-index: 10000001; "
	  + " } "
	).appendTo("body");

	notificationsDropdown.css({
		position: "absolute",   // Must be set before we can read width accurately
		"min-width": minWidth+"px",
		//overflow: "auto",
	}).appendTo("body"); // Done sooner so we can get its width
	var topOfDropdown = notificationButtonContainer.offset().top + notificationButtonContainer.innerHeight() + 4;
	var leftOfDropdown = notificationButtonContainer.offset().left + notificationButtonContainer.innerWidth()/2 - notificationsDropdown.innerWidth()/2;
	leftOfDropdown = Math.max(leftOfDropdown, 12);
	leftOfDropdown = Math.min(leftOfDropdown, window.innerWidth - 12 - notificationsDropdown.innerWidth() - 20);
	notificationsDropdown.css({
		top: topOfDropdown + "px",
		left: leftOfDropdown + "px",
		//"max-height": "calc(100% - "+(topOfDropdown+8)+"px)",
	});

	// This little white wedge should lead from the notification button to the title of the dropdown, +1 pixel lower in order to overlap the top border.
	tabArrow = $("<div>").addClass("notifications-dropdown-arrow").css({
		left: (notificationButtonContainer.offset().left + notificationButtonContainer.innerWidth()/2 - arrowSize) + "px",
		top: (topOfDropdown - arrowSize + 1) + "px",
	}).appendTo("body");

	makeNotificationBlocksCollapsable(notificationsDropdown);
	listenForMarkAsReadClick(notificationsDropdown);

	listenForCloseNotificationDropdown();
}

function listenForCloseNotificationDropdown(){
	closeClickTargets.on("click", considerClosingNotificiationDropdown);
}

function considerClosingNotificiationDropdown(evt){
	if ($(evt.target).closest(".notifications-dropdown").length){
		// A click inside the dropdown doesn't count!
	} else {
		evt.preventDefault();
		closeNotificationsDropdown();
		listenForNotificationClick();
	}
}

function closeNotificationsDropdown(){
	closeClickTargets.off("click", considerClosingNotificiationDropdown);
	notificationsDropdown.remove();
	tabArrow.remove();
	notificationButtonContainer.css({
		"background-color": "",
		"background-image": ""
	});
}

function listenForMarkAsReadClick(parentElement) {
	$(".mark-all-as-read", parentElement).click(function(){
		// Always collapse the repo's notifications block when the mark-as-read tick icon is clicked.
		var $divToCollapse = $(this).closest(".js-notifications-browser").find(".boxed-group-inner.notifications");
		collapseBlock($divToCollapse);
	});
}

function makeNotificationBlocksCollapsable(parentElement){
	makeBlocksCollapsable(parentElement, ".js-notifications-browser > h3", ".boxed-group-inner.notifications");
}

function makeFileAndDiffBlocksCollapsable(parentElement){
	makeBlocksCollapsable(parentElement, ".file.js-details-container > .meta", ".data.highlight");
}

// When an element matching headerSelector is clicked, the next sibling bodySelector will be collapsed or expanded (toggled).
function makeBlocksCollapsable(parentElement, headerSelector, bodySelector){
	$(headerSelector, parentElement).click(function(e){
		var $divToCollapse = $(this).next(bodySelector);
		var wasHidden = $divToCollapse.hasClass("ghndd-collapsed");
		var hideContent = !wasHidden;
		if (hideContent) {
			collapseBlock($divToCollapse);
		} else {
			expandBlock($divToCollapse);
		}
	}).css({ cursor: "pointer" });
}

// Under the new styling, while the top border is placed on the header, the bottom border is placed on the box.  (This is the case for notifications, but not for file/diff boxes.)
// If we hide the box entirely, we will lose the bottom border.
// So our plan is to rollup the box, hide its children, and then show the box again.
function collapseBlock($divToCollapse) {
	$divToCollapse.addClass("ghndd-collapsed");
	$divToCollapse.slideUp(150, function(){
		$divToCollapse.children().hide();
		$divToCollapse.slideDown(1);
	});
}
function expandBlock($divToCollapse) {
	$divToCollapse.removeClass("ghndd-collapsed");
	$divToCollapse.slideUp(1, function(){
		$divToCollapse.children().show();
		$divToCollapse.slideDown(150);
	});
}

function textNode(text){
	return document.createTextNode(text);
}


// Init:
listenForNotificationClick();

// Optional: If we are on the notifications page, add our rollup feature there too!
makeNotificationBlocksCollapsable(document.body);

// Optional: Also add the rollup feature for individual files on diff pages.
// TODO: This should be run on-demand, in case we reached a file or diff page via pushState().
makeFileAndDiffBlocksCollapsable(document.body);
