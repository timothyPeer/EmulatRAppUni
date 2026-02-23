hmLoadTopic({
hmKeywords:"",
hmTitle:"Systems Integration",
hmDescription:"Integrate via EmulatR  SYS-REQ-001: EmulatorManager MUST construct and own: ReservationManager, IRQController, SafeMemory, MMIOManager SYS-REQ-002: EmulatorManager MUST propag",
hmPrevLink:"",
hmNextLink:"",
hmParentLink:"appendix---trait-examples.html",
hmBreadCrumbs:"",
hmTitlePath:"Introduction > Appendix > Appendix I – Global Singletons",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">Systems Integration<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\"><span style=\"font-weight: bold;\">Integrate via EmulatR<\/span><\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<div style=\"text-align: left; text-indent: 0; padding: 0 0 0 0; margin: 0 0 0 0;\"><table style=\"border:none; border-spacing:0;\">\n\r<tr>\n\r<td style=\"vertical-align:top; width:6.7500rem; padding:0; border:none\"><p class=\"p_Normal\">SYS-REQ-001:<\/p>\n\r<\/td>\n\r<td style=\"vertical-align:top; width:48.8125rem; padding:0; border:none\"><p class=\"p_Normal\">EmulatorManager MUST construct and own: ReservationManager, IRQController, SafeMemory, MMIOManager<\/p>\n\r<\/td>\n\r<\/tr>\n\r<tr>\n\r<td style=\"vertical-align:top; width:6.7500rem; padding:0; border:none\"><p class=\"p_Normal\">SYS-REQ-002:<\/p>\n\r<\/td>\n\r<td style=\"vertical-align:top; width:48.8125rem; padding:0; border:none\"><p class=\"p_Normal\">EmulatorManager MUST propagate pointers to: Each AlphaCPU, DeviceManager, Memory subsystem, DMA engines<\/p>\n\r<\/td>\n\r<\/tr>\n\r<tr>\n\r<td style=\"vertical-align:top; width:6.7500rem; padding:0; border:none\"><p class=\"p_Normal\">SYS-REQ-003:<\/p>\n\r<\/td>\n\r<td style=\"vertical-align:top; width:48.8125rem; padding:0; border:none\"><p class=\"p_Normal\">Lifetime ordering: ReservationManager must outlive all CPUs, IRQController must outlive all CPUs, SafeMemory and MMIOManager must outlive ReservationManager<\/p>\n\r<\/td>\n\r<\/tr>\n\r<\/table>\n\r<\/div>\n\r"
})
