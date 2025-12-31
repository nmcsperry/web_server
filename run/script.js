const Socket = new WebSocket("ws://localhost");

Socket.onopen = function(Event)
{
    Socket.send("a");
}
Socket.onmessage = function(Event)
{
    const Deltas = JSON.parse(Event.data);
    for (let Index = 0; Index < Deltas.length; Index++)
    {
        ApplyDelta(Deltas[Index]);
    }
}

const HTMLDiff_TypeMask = 0xf;
const HTMLDiff_Tag = 0x1;
const HTMLDiff_Attr = 0x2;
const HTMLDiff_Style = 0x3;

const HTMLDiff_OperationMask = 0xf << 4;
const HTMLDiff_Insert = 0x1 << 4;
const HTMLDiff_Delete = 0x2 << 4;
const HTMLDiff_Replace = 0x3 << 4;
const HTMLDiff_Move = 0x7 << 4;
const HTMLDiff_ReplaceContent = 0x8 << 4;

function ApplyDelta(Delta)
{
    if (Delta.Invalid)
    {
        return;
    }

    let Element = document.getElementById(Delta.ID);
    if (Element == null)
    {
        console.log("There's going to be an error, because we could not find ID \"" + Delta.ID + "\"");
    }

    if ((Delta.Type & HTMLDiff_TypeMask) == HTMLDiff_Tag)
    {
        if ((Delta.Type & HTMLDiff_OperationMask) == HTMLDiff_Insert)
        {
            let NewElement = document.createRange().createContextualFragment(Delta.Content);
            Element.insertBefore(NewElement, Element.childNodes[Delta.Index]);
        }
        if ((Delta.Type & HTMLDiff_OperationMask) == HTMLDiff_Delete)
        {
            Element.remove();
        }
        if ((Delta.Type & HTMLDiff_OperationMask) == HTMLDiff_Replace)
        {
            Element.outerHTML = Delta.Content;
        }
        if ((Delta.Type & HTMLDiff_OperationMask) == HTMLDiff_Move)
        {
            let Parent = Element.parentNode;
            Parent.removeChild(Element);
            Parent.insertBefore(Element, Parent.childNodes[Delta.Index]);
        }
        if ((Delta.Type & HTMLDiff_OperationMask) == HTMLDiff_ReplaceContent)
        {
            Element.textContent = Delta.Content;
        }
    }
    else if ((Delta.Type & HTMLDiff_TypeMask) == HTMLDiff_Attr)
    {
        if (Delta.Content)
        {
            Element.setAttribute(Delta.AttrName, Delta.Content);
        }
        else
        {
            Element.removeAttribute(Delta.AttrName);
        }
    }
    else if ((Delta.Type & HTMLDiff_TypeMask) == HTMLDiff_Style)
    {
        if (Delta.Content)
        {
            Element.style[Delta.AttrName] = Delta.Content;
        }
        else
        {
            Element.style[Delta.AttrName] = null;
        }
    }
}