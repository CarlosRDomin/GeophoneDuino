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