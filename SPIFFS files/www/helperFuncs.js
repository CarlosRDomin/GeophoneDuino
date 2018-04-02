var numScriptsAdded = 0, numScriptsLoaded = 0;
function onLoadLibScript(e) {
	numScriptsLoaded++;	// Increase global counter
	if (numScriptsLoaded == numScriptsAdded) {	// If all scripts have loaded, call the $(document).ready(...)
		jQueryAssignDocumentReady();
	}
}

function loadScript(scriptURL) {
	var scriptTag = document.createElement('script');
	scriptTag.src = scriptURL;
	scriptTag.onload = onLoadLibScript;
	
	var backupIP = 'http://192.168.0.1/';
	if (window.location.protocol==='file:' && !scriptURL.startsWith(backupIP)) {
		scriptTag.onerror = function(e) { loadScript(backupIP + scriptURL); };
	}

	document.head.appendChild(scriptTag);
}

function addLibScriptSync(scriptRemoteURL, scriptLocalURL) {
	numScriptsAdded++;	// Increase global counter
	loadScript(testInternetConnectionSync(scriptRemoteURL)? scriptRemoteURL : scriptLocalURL);
}

function addLibScripts(listScriptURLs) {
	numScriptsAdded += listScriptURLs.length;	// Increase global counter
	testInternetConnectionAsync(listScriptURLs[0][0], function(success) {
		for (var i=0; i<listScriptURLs.length; ++i) {
			loadScript(listScriptURLs[i][success? 0:1]);	// Load either the internet script (listScriptURLs[i][0]) or the local copy (listScriptURLs[i][1])
		}
	});
}

function testInternetConnectionSync(testURL, addRandQuery=false) {
    var xhr = new XMLHttpRequest();
	var url = (addRandQuery? (testURL + "?rand=" + Math.round(Math.random()*10000)) : testURL);
    
	xhr.open('HEAD', url, false);	// Load synchronously
    try {
        xhr.send(null);
        if (xhr.status >= 200 && xhr.status < 400) {
            return true;
        } else {
            return false;
        }
    } catch (e) {
        return false;
    }
}

function testInternetConnectionAsync(testURL, onDone, timeout=2000, addRandQuery=false) {
    var xhr = new XMLHttpRequest();
	var url = (addRandQuery)? (testURL + "?rand=" + Math.round(Math.random()*10000)) : testURL;
    
	xhr.open('HEAD', url, true);	// Load async
	xhr.timeout = timeout;	// Add a timeout in ms so we don't wait forever
	xhr.onload = function(e) {
		if (xhr.readyState === 4) {
			if (xhr.status >= 200 && xhr.status < 400) {
				onDone(true);
			} else {
				onDone(false);
			}
		}
	};
	xhr.onerror = function (e) {
		onDone(false);
	};
	xhr.ontimeout = function (e) {
		onDone(false);
	};
	xhr.send(null);
}

// Helper functions to apply to arrays
Array.prototype.pushArray = function(arr) {
	this.push.apply(this, arr);
};

Array.prototype.sum = function() {
	return this.reduce(function(sum, a) { return sum + a }, 0);
};

Array.prototype.mean = function() {
	return this.sum() / (this.length||1);
};

// Log-related functions
function clearLog() {
	document.getElementById('textAreaLog').innerHTML = "";
}

function logMessage(msg, horizLine=false) {
	var msgEnd = (horizLine)? '<hr>':'<br>';	// Horizontal line already forces a new line
	document.getElementById('textAreaLog').innerHTML += "<span style='color: #999;'>" + (new Date()).toTimeString().substring(0,8) + " - </span>" + msg + msgEnd;
	if (document.getElementById('chkAutoScrollLog').checked) {
		document.getElementById('textAreaLog').scrollTop = document.getElementById('textAreaLog').scrollHeight;
	}
}

// WebSocket-related functions
function getStrCloseReason(eventCode) {
	switch (eventCode) {
		case 1000:
			return "Everything ok :)";
		case 1001:
			return "An endpoint is \"going away\", such as a server going down or a browser having navigated away from a page.";
		case 1002:
			return "An endpoint is terminating the connection due to a protocol error";
		case 1003:
			return "An endpoint is terminating the connection because it has received a type of data it cannot accept (e.g., an endpoint that understands only text data MAY send this if it receives a binary message).";
		case 1004:
			return "Reserved. The specific meaning might be defined in the future.";
		case 1005:
			return "No status code was actually present.";
		case 1006:
			return "The connection was closed abnormally, e.g., without sending or receiving a Close control frame";
		case 1007:
			return "An endpoint is terminating the connection because it has received data within a message that was not consistent with the type of the message (e.g., non-UTF-8 [http://tools.ietf.org/html/rfc3629] data within a text message).";
		case 1008:
			return "An endpoint is terminating the connection because it has received a message that \"violates its policy\". This reason is given either if there is no other sutible reason, or if there is a need to hide specific details about the policy.";
		case 1009:
			return "An endpoint is terminating the connection because it has received a message that is too big for it to process.";
		case 1011:
			return "A server is terminating the connection because it encountered an unexpected condition that prevented it from fulfilling the request.";
		case 1015:
			return "The connection was closed due to a failure to perform a TLS handshake (e.g., the server certificate can't be verified).";
		default:
			return "Unknown reason";
	}
}