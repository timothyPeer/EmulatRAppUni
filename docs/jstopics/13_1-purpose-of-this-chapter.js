hmLoadTopic({
hmKeywords:"",
hmTitle:"13.1 Purpose of This Chapter",
hmDescription:"This chapter documents the AlphaPipeline implementation: the internal execution conveyor that moves decoded instructions (\"grains\") from fetch through architectural retirement.",
hmPrevLink:"alphapipeline-implementation.html",
hmNextLink:"13_2-pipeline-role-and-design.html",
hmParentLink:"alphapipeline-implementation.html",
hmBreadCrumbs:"<a href=\"license-_-attributions.html\">ASA-EMulatR Reference Guide<\/a> &gt; <a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphapipeline-implementation.html\">Chapter 13 – AlphaPipeline Implementation<\/a>",
hmTitlePath:"ASA-EMulatR Reference Guide > Introduction > Architecture Overview > Chapter 13 – AlphaPipeline Implementation > 13.1 Purpose of This Chapter",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">13.1 Purpose of This Chapter<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">This chapter documents the AlphaPipeline implementation: the internal execution conveyor that moves decoded instructions (&quot;grains&quot;) from fetch through architectural retirement.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">This chapter answers: how instructions flow through the pipeline, how stalls\/flushes\/serialization are enforced, where architectural state becomes visible, and how correctness is guaranteed in the presence of speculation.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">This chapter does not define instruction semantics — those live in grains and execution boxes. It does not define the conceptual pipeline model — that is Chapter 3. This chapter documents the implementation mechanics.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">Implementation: AlphaPipeline.h (2,088 lines, fully inline header) and PipeLineSlot.h (409 lines).<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Chapter 3 – Pipeline Architecture (conceptual model); Chapter 12 – AlphaCPU Core (pipeline owner).<\/span><\/p>\n\r"
})
