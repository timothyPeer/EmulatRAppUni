hmLoadTopic({
hmKeywords:"",
hmTitle:"13.7 Stall Mechanics",
hmDescription:"A PipelineSlot may stall (slot.stalled = true) due to: memory barrier pending (needsMemoryBarrier && !memoryBarrierCompleted), write buffer drain pending (needsWriteBufferDrain...",
hmPrevLink:"13_6-stage-implementations.html",
hmNextLink:"13_8-flush-semantics.html",
hmParentLink:"alphapipeline-implementation.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphapipeline-implementation.html\">Chapter 13 – AlphaPipeline Implementation<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 13 – AlphaPipeline Implementation > 13.7 Stall Mechanics",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">13.7 Stall Mechanics<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">A PipelineSlot may stall (slot.stalled = true) due to: memory barrier pending (needsMemoryBarrier &amp;&amp; !memoryBarrierCompleted), write buffer drain pending (needsWriteBufferDrain &amp;&amp; !writeBufferDrained), multi-cycle FP execution (slot blocked in EX until complete), or device backpressure.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Stalls propagate forward only. If stage N stalls, stage N-1 cannot advance, earlier stages remain frozen, but later stages continue draining. This guarantees forward progress — older instructions always retire before younger ones can proceed.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">The stall is checked in execute() after all stages run: if isPipelineStalled() returns true, the ring buffer does not advance, and BoxResult::stallPipeline() is returned. The stalled stage will be re-evaluated on the next tick.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Section 6.5 – Pipeline Behavior (barrier stalls).<\/span><\/p>\n\r"
})
