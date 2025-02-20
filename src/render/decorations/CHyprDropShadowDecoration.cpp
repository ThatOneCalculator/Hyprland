#include "CHyprDropShadowDecoration.hpp"

#include "../../Compositor.hpp"

CHyprDropShadowDecoration::CHyprDropShadowDecoration(CWindow* pWindow) {
    m_pWindow = pWindow;
}

CHyprDropShadowDecoration::~CHyprDropShadowDecoration() {
    updateWindow(m_pWindow);
}

SWindowDecorationExtents CHyprDropShadowDecoration::getWindowDecorationExtents() {
    static auto *const PSHADOWS = &g_pConfigManager->getConfigValuePtr("decoration:drop_shadow")->intValue;

    if (*PSHADOWS != 1)
        return {{}, {}};

    return m_seExtents;
}

eDecorationType CHyprDropShadowDecoration::getDecorationType() {
    return DECORATION_SHADOW;
}

void CHyprDropShadowDecoration::damageEntire() {
    static auto *const PSHADOWS = &g_pConfigManager->getConfigValuePtr("decoration:drop_shadow")->intValue;

    if (*PSHADOWS != 1)
        return; // disabled

    wlr_box dm = {m_vLastWindowPos.x - m_seExtents.topLeft.x, m_vLastWindowPos.y - m_seExtents.topLeft.y, m_vLastWindowSize.x + m_seExtents.topLeft.x + m_seExtents.bottomRight.x, m_vLastWindowSize.y + m_seExtents.topLeft.y + m_seExtents.bottomRight.y};
    g_pHyprRenderer->damageBox(&dm);
}

void CHyprDropShadowDecoration::updateWindow(CWindow* pWindow) {
    damageEntire();

    const auto PWORKSPACE = g_pCompositor->getWorkspaceByID(pWindow->m_iWorkspaceID);

    const auto WORKSPACEOFFSET = PWORKSPACE && !pWindow->m_bPinned ? PWORKSPACE->m_vRenderOffset.vec() : Vector2D();

    if (pWindow->m_vRealPosition.vec() + WORKSPACEOFFSET != m_vLastWindowPos || pWindow->m_vRealSize.vec() != m_vLastWindowSize) {
        m_vLastWindowPos = pWindow->m_vRealPosition.vec() + WORKSPACEOFFSET;
        m_vLastWindowSize = pWindow->m_vRealSize.vec();

        damageEntire();
    }
}

void CHyprDropShadowDecoration::draw(CMonitor* pMonitor, float a, const Vector2D& offset) {

    if (!g_pCompositor->windowValidMapped(m_pWindow))
        return;

    if (m_pWindow->m_cRealShadowColor.col() == CColor(0, 0, 0, 0))
        return; // don't draw invisible shadows

    if (!m_pWindow->m_sSpecialRenderData.decorate)
        return;

    if (m_pWindow->m_sAdditionalConfigData.forceNoShadow)
        return;

    static auto *const PSHADOWS = &g_pConfigManager->getConfigValuePtr("decoration:drop_shadow")->intValue;
    static auto *const PSHADOWSIZE = &g_pConfigManager->getConfigValuePtr("decoration:shadow_range")->intValue;
    static auto *const PROUNDING = &g_pConfigManager->getConfigValuePtr("decoration:rounding")->intValue;
    static auto *const PSHADOWIGNOREWINDOW = &g_pConfigManager->getConfigValuePtr("decoration:shadow_ignore_window")->intValue;
    static auto *const PSHADOWOFFSET = &g_pConfigManager->getConfigValuePtr("decoration:shadow_offset")->vecValue;

    if (*PSHADOWS != 1)
        return; // disabled

    const auto ROUNDING = !m_pWindow->m_sSpecialRenderData.rounding ? 0 : (m_pWindow->m_sAdditionalConfigData.rounding == -1 ? *PROUNDING : m_pWindow->m_sAdditionalConfigData.rounding);

    // update the extents
    m_seExtents = {{*PSHADOWSIZE + 2 - PSHADOWOFFSET->x, *PSHADOWSIZE + 2 - PSHADOWOFFSET->y}, {*PSHADOWSIZE + 2 + PSHADOWOFFSET->x, *PSHADOWSIZE + 2 + PSHADOWOFFSET->y}};

    // draw the shadow
    wlr_box fullBox = {m_vLastWindowPos.x - m_seExtents.topLeft.x + 2, m_vLastWindowPos.y - m_seExtents.topLeft.y + 2, m_vLastWindowSize.x + m_seExtents.topLeft.x + m_seExtents.bottomRight.x - 4, m_vLastWindowSize.y + m_seExtents.topLeft.y + m_seExtents.bottomRight.y - 4};

    fullBox.x -= pMonitor->vecPosition.x;
    fullBox.y -= pMonitor->vecPosition.y;

    fullBox.x += offset.x;
    fullBox.y += offset.y;

    if (fullBox.width < 1 || fullBox.height < 1)
        return; // don't draw invisible shadows

    g_pHyprOpenGL->scissor((wlr_box *)nullptr);

    if (*PSHADOWIGNOREWINDOW) {
        glEnable(GL_STENCIL_TEST);

        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);

        glStencilFunc(GL_ALWAYS, 1, -1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        wlr_box windowBox = {m_vLastWindowPos.x - pMonitor->vecPosition.x, m_vLastWindowPos.y - pMonitor->vecPosition.y, m_vLastWindowSize.x, m_vLastWindowSize.y};

        scaleBox(&windowBox, pMonitor->scale);

        if (windowBox.width < 1 || windowBox.height < 1) {
            glClearStencil(0);
            glClear(GL_STENCIL_BUFFER_BIT);
            glDisable(GL_STENCIL_TEST);
            return;  // prevent assert failed
        }

        g_pHyprOpenGL->renderRect(&windowBox, CColor(0, 0, 0, 0), ROUNDING * pMonitor->scale);

        glStencilFunc(GL_NOTEQUAL, 1, -1);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }

    scaleBox(&fullBox, pMonitor->scale);
    g_pHyprOpenGL->renderRoundedShadow(&fullBox, ROUNDING * pMonitor->scale, *PSHADOWSIZE * pMonitor->scale, a);

    if (*PSHADOWIGNOREWINDOW) {
        // cleanup
        glClearStencil(0);
        glClear(GL_STENCIL_BUFFER_BIT);
        glDisable(GL_STENCIL_TEST);
    }
}
