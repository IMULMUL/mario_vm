// WritableStream self-check (WHATWG sink stream, minimal-but-functional).
let passed = 0, failed = 0;
function eq(got, want, msg) {
    if (got === want) passed++;
    else { failed++; console.log("FAIL " + msg + ": got " + JSON.stringify(got) + " want " + JSON.stringify(want)); }
}
function ok(cond, msg) { eq(!!cond, true, msg); }

// NB: this engine's `await` on a rejected promise yields undefined instead of
// throwing (a pre-existing VM limitation), so rejection is observed through a
// synchronous .then(onFulfilled, onRejected) - already-settled reactions run
// inline in the CLI. Returns {rejected, reason}.
function probe(p) {
    let out = { rejected: false, fulfilled: false, reason: null };
    p.then(() => { out.fulfilled = true; }, (e) => { out.rejected = true; out.reason = e; });
    return out;
}

// --- presence ---------------------------------------------------------------
eq(typeof WritableStream, "function", "WritableStream is a constructor");

async function main() {
    // --- basic write path ----------------------------------------------------
    let written = [];
    let started = false, closed = false;
    let ws = new WritableStream({
        start(c) { started = true; },
        write(chunk, c) { written.push(chunk); },
        close() { closed = true; }
    });
    eq(started, true, "start() ran during construction");
    eq(ws.locked, false, "unlocked before getWriter");
    ok(typeof ws.getWriter === "function", "getWriter present");

    let writer = ws.getWriter();
    eq(ws.locked, true, "locked after getWriter");
    ok(writer && typeof writer.write === "function", "writer.write present");

    await writer.write("a");
    await writer.write("b");
    eq(written.join(","), "a,b", "sink.write received both chunks in order");

    // write() resolves to a promise
    let p = writer.write("c");
    ok(p && typeof p.then === "function", "writer.write returns a thenable");
    await p;
    eq(written.length, 3, "third chunk written");

    // --- close ---------------------------------------------------------------
    await writer.close();
    eq(closed, true, "sink.close ran");

    // writing after close rejects
    let afterClose = probe(writer.write("d"));
    eq(afterClose.rejected, true, "write after close rejects");

    // --- locking: a second getWriter throws ---------------------------------
    let threw = false;
    try { ws.getWriter(); } catch (e) { threw = true; }
    eq(threw, true, "getWriter on a locked stream throws");

    // --- releaseLock then re-acquire ----------------------------------------
    let ws2 = new WritableStream({ write(chunk) { } });
    let w2 = ws2.getWriter();
    eq(ws2.locked, true, "ws2 locked");
    w2.releaseLock();
    eq(ws2.locked, false, "releaseLock unlocks the stream");
    let w2b = ws2.getWriter();
    ok(w2b && typeof w2b.write === "function", "can re-acquire a writer after releaseLock");

    // --- controller.error rejects the write and all later writes ------------
    let wsErr = new WritableStream({
        write(chunk, c) { c.error("boom"); }
    });
    let we = wsErr.getWriter();
    let first = probe(we.write("x"));
    eq(first.rejected, true, "the write that triggers controller.error rejects");
    eq(first.reason, "boom", "rejection carries the error reason");
    let second = probe(we.write("y"));
    eq(second.rejected, true, "write after controller.error rejects");

    // --- abort ---------------------------------------------------------------
    let aborted = null;
    let wsA = new WritableStream({ abort(r) { aborted = r; } });
    let wa = wsA.getWriter();
    await wa.abort("cancel-reason");
    eq(aborted, "cancel-reason", "sink.abort received the reason");

    // --- stream.close() / stream.abort() without an explicit writer ----------
    let closedDirect = false;
    let wsD = new WritableStream({ close() { closedDirect = true; } });
    await wsD.close();
    eq(closedDirect, true, "stream.close() invokes sink.close");

    // --- writer.closed / ready are promises ---------------------------------
    let wsR = new WritableStream({ write(c) {} });
    let wr = wsR.getWriter();
    ok(wr.closed && typeof wr.closed.then === "function", "writer.closed is a thenable");
    ok(wr.ready && typeof wr.ready.then === "function", "writer.ready is a thenable");

    console.log(failed === 0 ? "__WRITABLESTREAM_OK__" : ("__WRITABLESTREAM_FAIL__ " + failed));
    console.log("WritableStream: " + passed + " passed, " + failed + " failed");
}

main();
