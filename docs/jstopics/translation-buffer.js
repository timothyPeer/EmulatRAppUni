hmLoadTopic({
hmKeywords:"",
hmTitle:"Translation Buffer",
hmDescription:"L0 (silicon \/ SPAMShardManager) should never know how to walk page tables.",
hmPrevLink:"",
hmNextLink:"",
hmParentLink:"appendix---trait-examples.html",
hmBreadCrumbs:"",
hmTitlePath:"Introduction > Appendix > Appendix I – Global Singletons",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">Translation Buffer<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">L0 (silicon \/ SPAMShardManager) should never know how to walk page tables.<\/p>\n\r<p class=\"p_Normal\">L1 (Ev6TLBInterface) should only provide TLB operations: lookup\/insert\/invalidate.<\/p>\n\r<p class=\"p_Normal\">L2 (PAL\/MMU\/IPR layer) is where page-table walk logic like your walkPageTable belongs.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">L0 (&quot;silicon&quot;)<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Provides: tlbLookup, tlbInsert, invalidateVA, invalidateASN, etc.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">L1 (&quot;TLB interface&quot;)<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Wraps L0<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">No traits<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">No page-table knowledge<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">L2 (PAL \/ MMU \/ MISS handling)<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">On instruction\/data fetch: call L1 lookup<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">On miss: call walkPageTable_EV6()<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">On success: call L1 insert<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">On failure: raise TNV\/FOE\/FOW\/FOR<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">“yes, generate the L2 MMU handler”<\/p>\n\r"
})
