function activate_content(name)
{
    var node = document.getElementById(name);
	
    if (node.style.display == '')
    {
        node.style.display = 'block';
    }
    
    if (node.style.display != 'none')
    {
        node.style.display = 'none';
    }
    else
    {
        node.style.display = 'block';
    }
}

function collapse_tests()
{
    var nodes = document.getElementsByName("test-content");
    for (var i = 0; i < nodes.length; i++)
    {
        var node = nodes[i];
        activate_content(node.id);
    }
}
