const pint_glass_ml = 473;
const ml_per_pulse = 0.620921;

const glass_bottom_width = 160;
const glass_top_width = 220;
const glass_height = 380;
const glass_color = "#ccccccff";
const glass_outline_color = "#aaaaaaee";
//const beer_bottom_color = "#e59f49ff";
//const beer_top_color = "#f94b16ff";
const beer_bottom_color = "#cc6699ff";
const beer_top_color = "#bf4080ff"

var context = null;
var canvas_width = 0;
var canvas_height = 0;

var websocket = null;

function update_labels( pulses )
{
    var volume = pulses * ml_per_pulse;
    
    document.getElementById( "pulses" ).innerText = pulses.toString( );
    document.getElementById( "milliliters" ).innerText = volume.toFixed( 2 );

    draw_glass( volume );
}

function update_frequency( hz )
{
    document.getElementById( "flow" ).innerText = hz.toFixed( 4 );
}

function draw_glass( volume )
{
    if ( volume > pint_glass_ml )
    {
        volume = pint_glass_ml;
    }
    
    var area = ( volume / pint_glass_ml ) *
        ( ( glass_bottom_width + glass_top_width ) * glass_height / 2 );
    var delta_b = glass_top_width - glass_bottom_width;
    var quad_root = Math.sqrt( glass_height * ( glass_height * Math.pow( glass_bottom_width, 2 ) + 2 * area * delta_b ) );
    var beer_height = ( quad_root - glass_height * glass_bottom_width ) / delta_b;
    var beer_top_width = glass_bottom_width + ( beer_height * delta_b / glass_height );

    context.fillStyle = glass_color;
    context.strokeStyle = glass_outline_color;
    context.lineWidth = 3;
    
    context.beginPath( );
    context.moveTo( canvas_width/2 - glass_top_width/2, 0 );
    context.lineTo( canvas_width/2 + glass_top_width/2, 0 );
    context.lineTo( canvas_width/2 + glass_bottom_width/2, glass_height );
    context.lineTo( canvas_width/2 - glass_bottom_width/2, glass_height );
    context.closePath( );
    context.fill( );
    context.stroke( );

    var beer_gradient = context.createLinearGradient( 0, 0, 0, glass_height );
    beer_gradient.addColorStop( 0, beer_top_color );
    beer_gradient.addColorStop( 1, beer_bottom_color );
    context.fillStyle = beer_gradient;
    
    context.beginPath( );
    context.moveTo( canvas_width/2 - beer_top_width/2, glass_height - beer_height );
    context.lineTo( canvas_width/2 + beer_top_width/2, glass_height - beer_height );
    context.lineTo( canvas_width/2 + glass_bottom_width/2, glass_height );
    context.lineTo( canvas_width/2 - glass_bottom_width/2, glass_height );
    context.closePath( );
    context.fill( );
}

window.onload = function( )
{
    var canvas = document.getElementById( "glass" );
    context = canvas.getContext( "2d" );
    canvas_width = canvas.width;
    canvas_height = canvas.height;
    
    update_labels( 0 );

    websocket = new WebSocket( "ws://" + location.host + "/ws" );
    websocket.onmessage = function( event )
    {
        var match_pulse = event.data.match( /^pulses: (.+)/ );
        if ( match_pulse != null )
        {
            update_labels( parseInt( match_pulse[1] ) );
        }
        var match_flow = event.data.match( /^flow: (.+)/ );
        if ( match_flow != null )
        {
            update_frequency( parseFloat( match_flow[1] ) );
        }
    };
}
