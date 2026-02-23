hmLoadTopic({
hmKeywords:"",
hmTitle:"DeviceManager",
hmDescription:"DMA Coherence DEV-REQ-001: Any DMA write must call SafeMemory → ReservationManager. DEV-REQ-002: Devices must NOT bypass SafeMemory unless they explicitly call resConflictWrite",
hmPrevLink:"",
hmNextLink:"",
hmParentLink:"appendix---trait-examples.html",
hmBreadCrumbs:"",
hmTitlePath:"Introduction > Appendix > Appendix I – Global Singletons",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">DeviceManager<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\"><span style=\"font-weight: bold;\">DMA Coherence<\/span><\/p>\n\r<div style=\"text-align: left; text-indent: 0; padding: 0 0 0 0; margin: 0 0 0 0;\"><table style=\"border:none; border-spacing:0;\">\n\r<tr>\n\r<td style=\"vertical-align:top; width:6.6250rem; padding:0; border:none\"><p class=\"p_Normal\">DEV-REQ-001:<\/p>\n\r<\/td>\n\r<td style=\"vertical-align:top; width:48.8750rem; padding:0; border:none\"><p class=\"p_Normal\">Any DMA write must call SafeMemory → ReservationManager.<\/p>\n\r<\/td>\n\r<\/tr>\n\r<tr>\n\r<td style=\"vertical-align:top; width:6.6250rem; padding:0; border:none\"><p class=\"p_Normal\">DEV-REQ-002:<\/p>\n\r<\/td>\n\r<td style=\"vertical-align:top; width:48.8750rem; padding:0; border:none\"><p class=\"p_Normal\">Devices must NOT bypass SafeMemory unless they explicitly call resConflictWrite().<\/p>\n\r<\/td>\n\r<\/tr>\n\r<\/table>\n\r<\/div>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\"><span style=\"font-weight: bold;\">IRQ Integration<\/span><\/p>\n\r<div style=\"text-align: left; text-indent: 0; padding: 0 0 0 0; margin: 0 0 0 0;\"><table style=\"border:none; border-spacing:0;\">\n\r<tr>\n\r<td style=\"vertical-align:top; width:6.6250rem; padding:0; border:none\"><p class=\"p_Normal\">DEV-REQ-010:<\/p>\n\r<\/td>\n\r<td style=\"vertical-align:top; width:48.8750rem; padding:0; border:none\"><p class=\"p_Normal\">Devices must use IRQController to post interrupts (or level asserts).<\/p>\n\r<\/td>\n\r<\/tr>\n\r<tr>\n\r<td style=\"vertical-align:top; width:6.6250rem; padding:0; border:none\"><p class=\"p_Normal\">DEV-REQ-011:<\/p>\n\r<\/td>\n\r<td style=\"vertical-align:top; width:48.8750rem; padding:0; border:none\"><p class=\"p_Normal\">Devices must not directly modify CPU state.<\/p>\n\r<\/td>\n\r<\/tr>\n\r<\/table>\n\r<\/div>\n\r"
})
