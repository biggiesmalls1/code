package org.neuralyte.superproxy.grimeape;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.io.PrintStream;
import java.net.InetAddress;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.util.Hashtable;
import java.util.List;
import java.util.Map;
import java.util.Observable;
import java.util.Observer;

import org.apache.xerces.dom.CoreDocumentImpl;
import org.apache.xerces.dom.TextImpl;
import org.common.nap.Nap;
import org.neuralyte.Logger;
import org.neuralyte.common.FileUtils;
import org.neuralyte.common.io.StreamUtils;
import org.neuralyte.httpdata.HttpRequest;
import org.neuralyte.httpdata.HttpResponse;
import org.neuralyte.httpdatatools.HttpRequestBuilder;
import org.neuralyte.httpdatatools.HttpResponseBuilder;
import org.neuralyte.simpleserver.SocketServer;
import org.neuralyte.simpleserver.httpadapter.HTTPStreamingTools;
import org.neuralyte.superproxy.HTMLDOMUtils;
import org.neuralyte.superproxy.PluggableHttpRequestHandler;
import org.neuralyte.webserver.WebRequest;
import org.w3c.dom.Document;
import org.w3c.dom.Node;

import sun.misc.Signal;
import sun.misc.SignalHandler;

/**
 * GrimeApe aims to be a GreaseMonkey emulator which works as a web-proxy.
 */

/*
// TODO: If possible, rather than insert the script inline into the page it
// would be better
// to insert an external link to the script, so it can be loaded more
// intelligently (cached)
// by the browser.
// I guess these should be directed at the proxy, and the proxy should intercept
// them and handle them like a webserver would.
// Hmm in fact I would like to add one SCRIPT at the end of the page, which
// waits maybe a second for the page to really finish loading, then goes ahead
// and appends all our userscripts.

// @todo We should be checking each script's include/exclude patterns.

// @todo We should probably cache the scripts in memory, for speed, but update
// them if the file changes. include/exclude metas should be cached again (in
// fact we should have the regexps pre-compiled :).

// @todo We may be able to include some of the Firefox GM libraries, and get
// more userscripts working. Some functions might be portable if they don't work
// immediately.

// Hmm GM has browser privileges, and sandbox. We are running inside the page.
// This does mean a little "insecurity" - we are making our source available to
// the page, in theory it could be read and sent back to the site!

// Anyway we are much more limited than GM, but we could pull some tricks.

// We could implement GM_getValue and GM_setValue by making XMLHttpRequests
// (AJAX) to the proxy. GM will need to be high enough in the plugin chain to
// intercept these special http requests.

// Ah nice, we have opened up more insecurities. Now in theory the remote server
// could start fiddling with our proxy params!

// Since it's unlikely we will be able to really fix these, I recommend a
// "paranoid-mode" setting for users. They could choose to 'Always Allow'
// read-write to certain GM_setValue vars, choose 'Always Ask' for others. :)
*/

/*
 * TODO: When we get a 304 or 404 response. Well 304 is empty so we should NOT
 * inject! 404 sometimes has a message, so i guess injection is ok.
 * 
 * Possible advanced features of SuperProxy:
 * We could allow scripts to register to receive certain events from the Proxy,
 * such as 404? Would that be useful?
 */

public class GrimeApe extends PluggableHttpRequestHandler {

    // We don't only want to processDocuments.
    // Sometimes we may want to intercept HTTP requests,
    // e.g. the browser is requesting one of the scripts which we told it to
    // load on a previously response page.
    // OK so we don't need to do that.
    // But we do need the URL of the document we
    // are "processing". We should only add/run the relevant scripts with
    // matching includes/excludes.

    /*//// Intercepts ////
     * Requests made to /gRiMeApE/ are handled in special ways by the proxy.
     * /gRiMeApE/javascript/<script_name> to get a core script
     * /gRiMeApE/userscripts/<script_name> to get a userscript
     * e.g. /gRiMeApE/userscripts/config.js
     * /gRiMeApE/setValue?name=...&value=...
     * /gRiMeApE/getValue?name=...
     * OLD: /gRiMeApE/log?line=...
     * Request to other paths should produce an error, returning nothing useful.
     */
    
    static String topDir = ".";
    static File userscriptsDir = new File(topDir,"userscripts");
    /*
    static String coreScriptsDir = "./javascript/";
    static String userscriptsDir = "./userscripts/";
    */
    
    // TODO: This should not be a global, but associated with user/session.
    public static Map<String,String> gmRegistry = new Hashtable<String,String>();
    // TODO: Also it doesn't work in the daemon, because it does not persist on restart.

    public static void main(String[] args) throws Exception {
        
        if (args.length>0 && args[0].equals("--ind")) {
            doDaemonStreaming();
        } else {
            
            // I use 7152 for production, 7153 for dev. 
            int port = 7152; 
            if (args.length>=2 && args[0].equals("--port")) {
                port = Integer.parseInt(args[1]);
            }

            loadData();
            setupWatchForClose();

            new SocketServer(port,new GrimeApe()).run();
            
        }

    }

    private static void setupWatchForClose() {
        try {
            SignalHandler watchForClose = new SignalHandler() {
                public void handle(Signal sig) {
                    saveData();
                }
            };
            Observer observer = new Observer() {
                public void update(Observable arg0, Object arg1) {
                    saveData();
                }
            };
            /* oldHandler = */
            Signal.handle(new Signal("INT"), watchForClose);
            Signal.handle(new Signal("HUP"), watchForClose);
            Signal.handle(new Signal("QUIT"), watchForClose);
            Signal.handle(new Signal("KILL"), watchForClose);
            // Signal.handle(new Signal(""+SignalHandler.SIG_DFL), watchForClose);
            // Signal.handle(new Signal(""+SignalHandler.SIG_IGN), watchForClose);
        } catch (Throwable e) {
            Logger.warn("Problem setting up auto-save on close: "+e);
        }
    }

    public static void saveData() {
        /* if (interceptor.handle(sig.getName()) && (oldHandler != null)) {
            oldHandler.handle(sig);
        } */
        try {
            Logger.info("Shutting down...");
            Nap.writeToFile(gmRegistry, "grimeape_registry.nap");
        } catch (Exception e) {
            Logger.error(e);
        }
    }
    
    public static void loadData() throws Exception {
        try {
            gmRegistry = (Map<String,String>)Nap.fromFile("grimeape_registry.nap");
            Logger.log("Loaded "+gmRegistry.size()+" keys from file.");
        } catch (Exception e) {
            Logger.warn("Could not load registry: "+e);
        }
        if (gmRegistry == null) {
            gmRegistry = new Hashtable<String,String>();
        }
    }

    public static void doDaemonStreaming() throws Exception {
        
        /** Argh!  First thing we must do is stop logging from going to client output! **/
        OutputStream realOut = System.out;
        PrintStream logOut = new PrintStream(
                new FileOutputStream("/tmp/grimeaped.log",true)
        );
        // System.setOut(System.err); // Send logging to err out
        // Hmm still xinetd is sending err anyway.
        // System.setOut(null);
        // System.setErr(null);
        System.setOut(logOut);
        System.setErr(logOut);

        loadData();
        GrimeApe ga = new GrimeApe();
        ga.handleRequest(System.in, realOut); // Send response to real out
        saveData();
    }
        
    public HttpResponse handleHttpRequest(HttpRequest request) throws IOException {

        request.removeHeader("Proxy-Connection"); // Most proxies will want to do this, i.e. not pass this header to the remote host
        // Doing these because SimpleProxy can't yet handle keepalive :P
        request.removeHeader("Keep-Alive");
        request.setHeader("Connection", "close");
        request.setTopLine(request.getTopLine().replaceAll("HTTP/1.1$", "HTTP/1.0"));
        // Logger.log("New top line = "+request.getTopLine());
        
        //// Check for special requests directed at GrimeApe, not the web.
        WebRequest wreq = new WebRequest(request);
        // Logger.warn("path = " + wreq.getPath());
        if (wreq.getPath().startsWith("/_gRiMeApE_/")) {
            return handleSpecialRequest(request, wreq);
        }
        
        // All our proxies should check for this.
        // If the target of the client request is the proxy,
        // we will get stuck in a loop trying to proxy that request!
        String localhost = "127.0.0.1"; // Was giving me 127.0.1.1 hence failing to match: InetAddress.getLocalHost().getHostAddress();
        String reqHost= InetAddress.getByName(wreq.getHost()).getHostAddress();
        if (reqHost.equals(localhost)) {
            // throw new Error("Requested host is me!");
            return HttpResponseBuilder.stringHttpResponse("text/txt", "I am a proxy.  Please leave me alone.");
        }
        
        //// OK we have a normal web request.
        //// Handle it, then inject scripts if it is a web page.
        
        // HttpResponse response = HTTPStreamingTools.passRequestToServer(request);
        HttpResponse response = super.handleHttpRequest(request);
        
        /** TODO: I think we have a bug here, might be more common that just GrimeApe.
         * If we *didn't* change HTTP/1.1 to 1.0 earlier, and receive o 404 Not Found
         * then the connection seems to stay open for ages.
         * I think this may be because some part of the system is expecting a contentStream.
         */

        /** Actually that bug might not exist.  The problem is the browser was spinning the
         * "loading" icon, although it seemed all request had finished.
         * I think the problem there was that I had broken gmRegistry==null, cause a NPE,
         * so GM_setValue requests were not returning a valid response.
         * Therefore...
         */
        
        /** @todo BUG
         * If an exception(error/throwable) is thrown, should catch it and at least return
         * a valid response, "391.5 ERROR" or whatever.
         * (Well usually we should.  Maybe attempted access violations we shouldn't even. :P ) 
         */
        
        maybeAddScripts(response);
        
        return response;
    }

    private HttpResponse handleSpecialRequest(HttpRequest request, WebRequest wreq)
            throws Error {
        Logger.log("Handling special GA request: "+wreq.getPath());
        String[] args = wreq.getPath().split("/");
        // We know args[1] == "_gRiMeApE_"
        String commandDir = args[2];
        // Logger.warn("args[1] = " + args[1]);
        
        if (
                commandDir.equals("javascript")
                || commandDir.equals("userscripts")
                || commandDir.equals("images")
        ) {
            /* For these paths we simply serve the requested file. */
            /** DONE I think: Security danger of "../../../../etc/passwd" in args3,4,... !
             * (Although Konqueror won't usually let you do this, other things might.) 
            **/

            String fileBeyond = wreq.getPath().replaceAll("^/[^/]*/[^/]*/*", ""); // aka args[3] onward, joined again.

            // Check security by checking requested dir does lie beneath validScriptDir.
            File jailDir = new File(topDir,commandDir); // We can trust commandDir, we checked it.
            File requestedFile = new File(jailDir,fileBeyond);
            assertFileIsBelow(requestedFile, jailDir);
            // File checkParent = scriptFile.getParentFile();
            return HttpResponseBuilder.makeFileHttpResponse(requestedFile,"text/javascript",request);
            
        } else if (commandDir.equals("log")) {
            // We don't use this, it's very slow to send all logging to the server.
            // Now we prefer to keep it on the client and display it with Javascript.
            String logData = wreq.getParam("data");
            logData = URLDecoder.decode(logData);
            Logger.info("GM_LOG: "+logData);
            return noData();

        } else if (commandDir.equals("setValue")) {
            String name = wreq.getParam("name");
            String value = wreq.getParam("value");
            Logger.info("GM_SETVALUE: "+name+" = \""+value+"\"");
            gmRegistry.put(name,value);
            // @todo Occasional saves - remove this later.
            if (Math.random() < 0.1) {
                saveData();
            }
            return noData();
            
        } else if (commandDir.equals("getValue")) {
            String name = wreq.getParam("name");
            String value = gmRegistry.get(name);
            Logger.info("GM_GETVALUE: "+name+" ~ \""+value+"\"");
            String type = wreq.getParam("type");
            // Return raw (if request was an XMLHttpRequest):
            String response = ""+value;
            // Or return JS (if request was made by adding new <SCRIPT> to doc):
            if (type.equals("js")) {
                // We don't really use this, although it is an alternative response method.
                // The calling Javascript must wait on a Timer.
                response =  "window.GM_getValueResult = " + (
                        value == null
                        ? "null"
                                : "\"" + URLEncoder.encode(value)+"\""
                ) + ";";
            } else {
                // Konqueror was happy receiving just the String, but Firefox needs
                // something well-formed... // sob
                response = "<RESPONSE>" + URLEncoder.encode(response.replaceAll("\\+","%2b")) + "</RESPONSE>";
            }
            return HttpResponseBuilder.stringHttpResponse("text/xml",response);

        } else if (commandDir.equals("saveAll")) {
            saveData();
            return noData();

        } else if (commandDir.equals("clearAll")) {
            gmRegistry.clear();
            saveData();
            return noData();

        } else if (commandDir.equals("installUserscript")) {
            String name = wreq.getParam("name");
            String fsName = name.toLowerCase().replaceAll(" ", "_");
            String url = wreq.getParam("url");
            String contents = wreq.getParam("contents");
            File outFile = new File(userscriptsDir,fsName+"/"+fsName+".user.js");
            assertFileIsBelow(outFile,userscriptsDir);
            
            HttpRequest req = HttpRequestBuilder.getResource(url);
            HttpResponse res = HTTPStreamingTools.passRequestToServer(req);
            outFile.getParentFile().mkdir();
            try {
                OutputStream out = new FileOutputStream(outFile);
                StreamUtils.pipeStream(res.getContentAsStream(), out);
            } catch (Exception e) {
                Logger.error(e);
                return HttpResponseBuilder.stringHttpResponse("text/xml","<%!CDATA>Failure</%CDATA>");
            }
            return HttpResponseBuilder.stringHttpResponse("text/xml","<%!CDATA>Success!</%CDATA>");
            
        } else {
            Logger.error("Bad request: "+wreq.getPath());
            throw new Error("Bad request: "+wreq.getPath());
        }
        
    }
    
    public static void assertFileIsBelow(File suspect, File parent) {
        if (!fileIsBelow(suspect, parent)) {
            throw new Error("Attempted Security Breach Detected! Requested file \""+suspect+"\" is not an ancestor of \""+parent+"\".");
        }
    }

        public static boolean fileIsBelow(File suspect, File parent) {
        return (suspect.toString().startsWith(parent.toString()));
    }

    private HttpResponse noData() {
        return HttpResponseBuilder.stringHttpResponse("text/xml","<NODATA>OK</NODATA>");
    }

    private void maybeAddScripts(HttpResponse response) throws IOException {
        /** @todo What does GM trigger on?  I've seen it run (fail) on .txt and .js pages also. **/
        if (response.getHeader("Content-type").toLowerCase().startsWith("text/html")) {
            StringBuffer responseString = StreamUtils.streamStringBufferFrom(response.getContentAsStream());
            // Logger.log(""+responseString);
            int i = responseString.lastIndexOf("</BODY>");
            if (i == -1)
                i = responseString.lastIndexOf("</body>");
            if (i == -1) {
                i = responseString.length();
                // yeah nice Google doesn't bother with </BODY> lol
            }
            if (i == -1) {
                Logger.warn("Failed to inject script tag.");
            } else {
                Logger.log("Doing injection at index "+i);
                String[] scriptsToInject = {
                        "javascript/grimeape_greasemonkey_compat.js",
                        // "javascript/test.js",
                        "javascript/grimeape_config.js",
                        // "userscripts/faviconizegoogle/faviconizegoogle.user.js",
                        // "userscripts/track_history/track_history.user.js",
                        // "userscripts/alert_watcher/alert_watcher.user.js",
                        // "userscripts/reclaim_cpu/reclaim_cpu.user.js",
                        // "userscripts/auto_highlight_text_on_a/auto_highlight_text_on_a.user.js",
                };
                for (String script : scriptsToInject) {
                    // String srcURL = "/_gRiMeApE_/javascript/test.js";
                    String srcURL = "/_gRiMeApE_/"+script;
                    String injectHTML = "<SCRIPT type='text/javascript' src='" + srcURL + "'></SCRIPT>\n";
                    responseString.insert(i, injectHTML);
                    i += injectHTML.length();
                }
            }
            // We must reset it after streaming it, even if we didn't change it.
            response.setContent(responseString.toString());
        }
    }

}
