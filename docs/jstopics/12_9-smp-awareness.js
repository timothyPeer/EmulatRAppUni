hmLoadTopic({
hmKeywords:"",
hmTitle:"12.9 SMP Awareness",
hmDescription:"Each AlphaCPU knows its CPUId (m_cpuId, assigned at construction), participates in SMP coordination via injected references (m_reservationManager, m_ipiManager,...",
hmPrevLink:"12_8-pal-integration.html",
hmNextLink:"12_10-cpu-local-events.html",
hmParentLink:"alphacpu-core.html",
hmBreadCrumbs:"<a href=\"license-_-attributions.html\">ASA-EMulatR Reference Guide<\/a> &gt; <a href=\"index.html\">Introduction<\/a> &gt; <a href=\"architecture-overview.html\">Architecture Overview<\/a> &gt; <a href=\"alphacpu-core.html\">Chapter 12 – AlphaCPU Core<\/a>",
hmTitlePath:"ASA-EMulatR Reference Guide > Introduction > Architecture Overview > Chapter 12 – AlphaCPU Core > 12.9 SMP Awareness",
hmHeader:"<h1 class=\"p_Heading1\" style=\"page-break-after: avoid;\"><span class=\"f_Heading1\">12.9 SMP Awareness<\/span><\/h1>\n\r",
hmBody:"<p class=\"p_Normal\">Each AlphaCPU knows its CPUId (m_cpuId, assigned at construction), participates in SMP coordination via injected references (m_reservationManager, m_ipiManager, m_memoryBarrierCoordinator), receives IPIs as interrupts (via m_pending\/m_router), and performs local TLB invalidation via handleTLBShootdownIPI().<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">handleTLBShootdownIPI(cpuId, ipiData) decodes the IPICommand from the IPI data and dispatches to the appropriate TLB invalidation: TLB_INVALIDATE_VA_ITB → m_tlb→invalidateTLBEntry(cpuId, Realm::I, va, asn), TLB_INVALIDATE_VA_DTB → Realm::D, TLB_INVALIDATE_ASN → m_tlb→invalidateTLBsByASN(cpuId, asn), TLB_INVALIDATE_ALL → m_tlb→invalidateAllTLBs(cpuId).<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_Normal\">AlphaCPU never directly modifies another CPU\'s state. All cross-CPU effects occur through IPIs (which the target CPU processes in its own run loop) or through shared state (ReservationManager, GuestMemory) with proper atomic operations.<\/p>\n\r<p class=\"p_Normal\">&nbsp;<\/p>\n\r<p class=\"p_SeeAlso\" style=\"page-break-after: avoid;\"><span class=\"f_SeeAlso\">See Also: <a href=\"chapter9-smparchitecture.html\" class=\"topiclink\">Chapter 9 - SMP Architecture<\/a>.<\/span><\/p>\n\r"
})
