/*
 * Copyright (C) 2013-2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.ImageResourceContentView = class ImageResourceContentView extends WebInspector.ResourceContentView
{
    constructor(resource)
    {
        super(resource, "image");

        this._imageElement = null;

        const toolTip = WebInspector.UIString("Show Grid");
        const activatedToolTip = WebInspector.UIString("Hide Grid");
        this._showGridButtonNavigationItem = new WebInspector.ActivateButtonNavigationItem("show-grid", toolTip, activatedToolTip, "Images/NavigationItemCheckers.svg", 13, 13);
        this._showGridButtonNavigationItem.addEventListener(WebInspector.ButtonNavigationItem.Event.Clicked, this._showGridButtonClicked, this);
        this._showGridButtonNavigationItem.activated = !!WebInspector.settings.showImageGrid.value;
    }

    // Public

    get navigationItems()
    {
        return [this._showGridButtonNavigationItem];
    }

    contentAvailable(content, base64Encoded)
    {
        let objectURL = this.resource.createObjectURL();
        if (!objectURL) {
            this.showGenericErrorMessage();
            return;
        }

        this.element.removeChildren();

        this._imageElement = document.createElement("img");
        this._imageElement.addEventListener("load", function() { URL.revokeObjectURL(objectURL); });
        this._imageElement.src = objectURL;
        this._imageElement.setAttribute("filename", this.resource.urlComponents.lastPathComponent || "");
        this._updateImageGrid();

        this.element.appendChild(this._imageElement);
    }

    // Protected

    shown()
    {
        super.shown();

        this._updateImageGrid();

        WebInspector.settings.showImageGrid.addEventListener(WebInspector.Setting.Event.Changed, this._updateImageGrid, this);
    }

    hidden()
    {
        WebInspector.settings.showImageGrid.removeEventListener(WebInspector.Setting.Event.Changed, this._updateImageGrid, this);

        super.hidden();
    }

    // Private

    _updateImageGrid()
    {
        if (!this._imageElement)
            return;

        let activated = WebInspector.settings.showImageGrid.value;
        this._showGridButtonNavigationItem.activated = activated;
        this._imageElement.classList.toggle("show-grid", activated);
    }

    _showGridButtonClicked(event)
    {
        WebInspector.settings.showImageGrid.value = !this._showGridButtonNavigationItem.activated;

        this._updateImageGrid();
    }
};
