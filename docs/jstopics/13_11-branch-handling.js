hmLoadTopic({
hmKeywords:"",
hmTitle:"13.11 Branch Handling",
hmDescription:"Branches are resolved in EX stage. IBox predicts the target during fetch and stores the prediction in slot.predictionTarget\/predictionValid. In EX, the actual branch condition...",
hmPrevLink:"13_10-exception-precision.html",
hmNextLink:"13_12-ll_sc-and-determinism.html",
hmParentLink:"alphapipeline-implementation.html",
hmBreadCrumbs:"<a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphapipeline-implementation.html\">Chapter 13 – AlphaPipeline Implementation<\/a>",
hmTitlePath:"Introduction > Architecture Overview > Chapter 13 – AlphaPipeline Implementation > 13.11 Branch Handling",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">13.11 Branch Handling<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">Branches are resolved in EX stage. IBox predicts the target during fetch and stores the prediction in slot.predictionTarget\/predictionValid. In EX, the actual branch condition is evaluated by the grain. If slot.branchTaken is true and the actual target differs from the predicted target, a misprediction is flagged (slot.flushPipeline = true).<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">The execute() method checks for flushPipeline after stage_EX() — younger stages (IF, DE, IS) are invalidated and the correct PC is used for the next fetch. Branch predictor tables are updated in stage_WB() via m_cBox→updatePrediction(pc, taken, target).<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">PC is updated only in WB via retirement — no earlier stage modifies the architectural PC.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: Section 3.9 – Branch Resolution.<\/span><\/p>\n\r"
})
