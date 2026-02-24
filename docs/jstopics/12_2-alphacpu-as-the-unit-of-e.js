hmLoadTopic({
hmKeywords:"",
hmTitle:"12.2 AlphaCPU as the Unit of Execution",
hmDescription:"Each AlphaCPU instance represents one physical Alpha processor. One AlphaCPU = one hardware CPU. Each AlphaCPU runs independently in its own QThread. All AlphaCPUs execute...",
hmPrevLink:"12_1-purpose-of-this-chapter.html",
hmNextLink:"12_3-ownership-model.html",
hmParentLink:"alphacpu-core.html",
hmBreadCrumbs:"<a href=\"license-_-attributions.html\">ASA-EMulatR Reference Guide<\/a> &gt; <a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphacpu-core.html\">Chapter 12 – AlphaCPU Core<\/a>",
hmTitlePath:"ASA-EMulatR Reference Guide > Introduction > Architecture Overview > Chapter 12 – AlphaCPU Core > 12.2 AlphaCPU as the Unit of Execution",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">12.2 AlphaCPU as the Unit of Execution<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">Each AlphaCPU instance represents one physical Alpha processor. One AlphaCPU = one hardware CPU. Each AlphaCPU runs independently in its own QThread. All AlphaCPUs execute concurrently in SMP. AlphaCPU owns all per-CPU state. There is no shared CPU object and no master CPU.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">AlphaCPU is declared as a QObject (for Qt signal\/slot threading and MOC support), with alignas(8) for cache-line-friendly member layout:<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_CodeExample\"><span class=\"f_CodeExample\">class&nbsp;alignas(8)&nbsp;AlphaCPU&nbsp;final&nbsp;:&nbsp;public&nbsp;QObject&nbsp;{<\/span><\/p>\n\r<p class=\"p_CodeExample\"><span class=\"f_CodeExample\">&nbsp;Q_OBJECT<\/span><\/p>\n\r<p class=\"p_CodeExample\"><span class=\"f_CodeExample\">&nbsp;...<\/span><\/p>\n\r<p class=\"p_CodeExample\"><span class=\"f_CodeExample\">};<\/span><\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: <a href=\"chapter-9_3-cpu-instantiation-.html\" class=\"topiclink\">9.3 CPU Instantiation and Identity<\/a>.<\/span><\/p>\n\r"
})
