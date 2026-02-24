hmLoadTopic({
hmKeywords:"",
hmTitle:"10.11 PAL and Device Control",
hmDescription:"Some device operations require PAL mode: device initialization, interrupt routing configuration, DMA enablement, and system-wide device control. Accessing these privileged...",
hmPrevLink:"10_10-mmio-and-serialization.html",
hmNextLink:"10_12-smp-and-error-handling.html",
hmParentLink:"chapter-10---devices-and-mmio.html",
hmBreadCrumbs:"<a href=\"license-_-attributions.html\">ASA-EMulatR Reference Guide<\/a> &gt; <a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"chapter-10---devices-and-mmio.html\">Chapter 10 – Devices and Memory-Mapped I\/O (MMIO)<\/a>",
hmTitlePath:"ASA-EMulatR Reference Guide > Introduction > Architecture Overview > Chapter 10 – Devices and Memory-Mapped I\/O (MMIO) > 10.11 PAL and Device Control",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">10.11 PAL and Device Control<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">Some device operations require PAL mode: device initialization, interrupt routing configuration, DMA enablement, and system-wide device control. Accessing these privileged registers outside PAL faults with OPCDEC.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">All device interrupts ultimately enter PAL. PAL code identifies the interrupt source, acknowledges the device (typically via MMIO register write), invokes the OS handler, and returns via HW_REI. This ensures controlled, serialized device interaction.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: <a href=\"chapter-8---pal-and-privleged-.html\" class=\"topiclink\">Chapter 8 - PAL and Privileged Boundary<\/a>.<\/span><\/p>\n\r"
})
