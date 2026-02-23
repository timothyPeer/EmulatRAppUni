hmLoadTopic({
hmKeywords:"",
hmTitle:"IPI - Inter Processor Interrupt Handling",
hmDescription:"CPU 0 executes MTPR IPIR (target=CPU 1)     ↓ 1. Queue IPI message in CPU 1\'s IPIQueue 2. Request interrupt via IRQ controller at IPI_IPL     ↓ CPU 1 receives interrupt at",
hmPrevLink:"",
hmNextLink:"",
hmParentLink:"appendix---trait-examples.html",
hmBreadCrumbs:"",
hmTitlePath:"Introduction > Appendix > Appendix I – Global Singletons",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">IPI - Inter Processor Interrupt Handling<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">CPU 0 executes MTPR IPIR (target=CPU 1)<\/p>\n\r<p class=\"p_Normal\"> &nbsp; &nbsp;↓<\/p>\n\r<p class=\"p_Normal\">1. Queue IPI message in CPU 1\'s IPIQueue<\/p>\n\r<p class=\"p_Normal\">2. Request interrupt via IRQ controller at IPI_IPL<\/p>\n\r<p class=\"p_Normal\"> &nbsp; &nbsp;↓<\/p>\n\r<p class=\"p_Normal\">CPU 1 receives interrupt at IPI_IPL<\/p>\n\r<p class=\"p_Normal\"> &nbsp; &nbsp;↓<\/p>\n\r<p class=\"p_Normal\">CPU 1\'s interrupt handler:<\/p>\n\r<p class=\"p_Normal\"> &nbsp; &nbsp;- Reads IPI message from queue<\/p>\n\r<p class=\"p_Normal\"> &nbsp; &nbsp;- Processes IPI (TLB shootdown, etc.)<\/p>\n\r<p class=\"p_Normal\"> &nbsp; &nbsp;- Clears interrupt<\/p>\n\r"
})
