package jlib.dnd.hack;

// javaJVMLocalObjectMimeType 

import java.awt.*;

import jlib.*;
import jlib.simple.*;
import jlib.dnd.hack.*;

/**
 *
 * Simplistic use of drag and drop to transport data between components in a
 * single JVM.  Assumes only one mouse!  Does not interact with proper Java
 * drag and drop.
 *
 * For any Component c you wish to be a source for DnD, do
 *   DnDManager.setSource(c,o);
 * where the source Object o is the information about the source which the
 * target will receive.
 *
 * For target components there are two alternative methods:
 *
 * Method A:  You could implement a ComponentAcceptingDrop, which must provide
 * the method
 * 	 void drop(Object o) { ... }
 * Your component can then be intialised as a drop target using
 *   DnDManager.setTarget(c);
 * Then if a drag-and-drop ends on this component, the drop() method will be
 * called with the drag's source object provided as its argument.
 *
 * Method B: You could use a normal Component as your drop target.
 * In this case you must initialise the drop target Component with:
 * 	 DnDManager.setTarget(component,code);
 * where Code code is a simple class in which you are expected to overwrite
 * the execute() method and use the input field, which will be set to the
 * drag's source object before execute() is called.
 *
**/

// For Method A
class MyDropLabel extends Label implements ComponentAcceptingDrop {
	MyDropLabel(String s) {
		super(s);
	}
	public void drop(Object o) {
		System.out.println("MyDropLabel received:");
		System.out.println("    "+o);
	}
}

public class JoeyDD {

	public static void main(String[] argv) {

		Frame f=new Frame("D+D");
   	f.setSize(400,250);
		f.setLayout(new FlowLayout());

		// A normal label with a dnd source object attached.
		Label src=new Label("Pick me up!");
		DnDManager.setSource(src,"An example source object (a rather boring String)");
		f.add(src);

		// Method A
		// A special label which implements ComponentAcceptingDrop.
		// When a drag and drop ends on this target Component,
		//   its drop( ) method will be called with the drag's source Object.
		MyDropLabel dest=new MyDropLabel("MyDropLabel");
		DnDManager.setTarget(dest);
		f.add(dest);

		// Method B
		// A normal label with some target code attached to it.
		// When a drag and drop ends on this target Component:
		// 	 the Code's input field will be set to the drag's source Object, and
		// 	 the Code's execute() method will be called.
		Label normallabel=new Label("A normal Label");
		Code attachCode=new Code() {
			public void execute() {
				System.out.print("This was dropped on the normal Label:\n  "+input+"\n");
			}
		};
		DnDManager.setTarget(normallabel,attachCode);
		f.add(normallabel);

		// Let's also make that last component another source.
		// To show we can pass any object, set the source object to be the Frame.
		DnDManager.setSource(normallabel,f);

		f.setVisible(true);

	}

}
