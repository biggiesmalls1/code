package tools.parser;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Vector;



// @done Replace the Vector with a Hashtable, so that later atom definitions can
// overwrite earlier ones.

public class Grammar {

	// public static Vector rulesets=new Vector();

	/*public static HashMap<String, RuleSet> rulesets = new HashMap<String, RuleSet>() {
		// For backwards-compat, we provide .add().  (This used to be a vector!)
		void add(RuleSet rs) {
			put(rs.type,rs);
		}
	};*/

	public Map<String, RuleSet> rulesets = new HashMap<String, RuleSet>();

	public Grammar() {
	}
	
	public Grammar(Map<String,RuleSet> _rulesets) {
		rulesets = _rulesets;
	}

	public RuleSet getrulesetforatom(String type, ParseContext ctx) {
		/*
		for (int i=0;i<rulesets.size();i++) {
			RuleSet rs=(RuleSet)rulesets.get(i);
			if (rs.type.equals(type))
				return rs;
		}
		System.out.println("Grammar.getrulesetforatom("+type+"): not found!");
		System.exit(1);
		return null;
		*/
		return rulesets.get(type);
	}


	public void addRuleset(RuleSet ruleset) {
		// rulesets.add(ruleset);
		rulesets.put(ruleset.type, ruleset);
	}


	public void importNamespace(String namespace, String file) {
		// TODO Auto-generated method stub

	}


	public void setOption(String name, String[] args) {

	}

	List<PreProcessor> preProcessors = new Vector<PreProcessor>();


	public void addPreProcessor(PreProcessor preProcessor) {
		preProcessors.add(preProcessor);
	}

}
