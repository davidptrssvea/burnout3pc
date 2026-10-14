using System;

using System.Collections.Generic;

using System.Diagnostics;

using System.Drawing;

using System.IO;

using System.Text;

using System.Windows.Forms;



class ControlsForm : Form {

    readonly string dir=AppDomain.CurrentDomain.BaseDirectory;

    readonly Dictionary<string,string> settings=new Dictionary<string,string>();

    readonly string[] keys={"select","l3","r3","start","up","right","down","left","l2","r2","l1","r1","triangle","circle","cross","square"};

    readonly string[] original={"view","l3","r3","menu","up","right","down","left","lt","rt","lb","rb","y","b","a","x"};

    readonly string[] modern={"view","l3","r3","menu","up","right","down","left","x","rb","lb","a","y","b","a+rt","lt"};

    readonly string[] afValues={"off","2x","4x","8x","16x"};

    readonly ComboBox[] maps=new ComboBox[16];

    readonly NumericUpDown rumble=new NumericUpDown(), threshold=new NumericUpDown();

    readonly ComboBox aa=new ComboBox(), af=new ComboBox();

    readonly CheckBox fps=new CheckBox();

    readonly CheckBox prompts=new CheckBox();

    readonly CheckBox performance=new CheckBox();

    string Get(string key,string fallback) { string value; return settings.TryGetValue(key,out value)?value:fallback; }

    Label LabelAt(string text,int y) { var l=new Label { Text=text }; l.SetBounds(16,y,270,26); Controls.Add(l); return l; }

    Button ButtonAt(string text,int x,int width,EventHandler click) { var b=new Button {Text=text}; b.SetBounds(x,753,width,38); b.Click+=click; Controls.Add(b); return b; }

    int Percent(string key,int fallback) { int n; return int.TryParse(Get(key,""),out n)?Math.Max(0,Math.Min(100,n)):fallback; }

    public ControlsForm() {

        string path=Path.Combine(dir,"pc_settings.ini");

        if(File.Exists(path)) foreach(string line in File.ReadAllLines(path)) {

            int equals=line.IndexOf('=');

            if(equals>0 && !line.StartsWith("#")) settings[line.Substring(0,equals).Trim()]=line.Substring(equals+1).Trim();

        }

        Text="Burnout 3 – Controls and Graphics"; ClientSize=new Size(520,814);

        StartPosition=FormStartPosition.CenterScreen; FormBorderStyle=FormBorderStyle.FixedDialog;

        MaximizeBox=false; Font=new Font("Segoe UI",10);

        var note=new Label { Text="Close the game first. Use a+rt for alternative buttons. Button prompts update the next time you start the game." };

        note.SetBounds(16,12,485,50); Controls.Add(note);

        string[] labels={"Select","L3","R3","Start / Pause","D-pad Up","D-pad Right","D-pad Down","D-pad Left","L2","R2","L1","R1 / Boost","Triangle / Back","Circle / Color and Extra Options","Cross / Accelerate and Confirm","Square / Brake"};

        for(int i=0;i<16;i++) {

            LabelAt(labels[i],66+i*28);

            var box=new ComboBox(); box.SetBounds(292,63+i*28,210,26);

            box.Items.AddRange(original); box.Items.Add("a+rt"); box.Items.Add("none");

            box.Text=Get("map."+keys[i],original[i]); Controls.Add(box); maps[i]=box;

        }

        LabelAt("Vibration Strength (%)",518); rumble.SetBounds(292,514,90,28);

        rumble.Minimum=0; rumble.Maximum=100; rumble.Value=Percent("rumble_strength",100); Controls.Add(rumble);

        LabelAt("Trigger Activation Threshold (%)",553); threshold.SetBounds(292,548,90,28);

        threshold.Minimum=0; threshold.Maximum=100; threshold.Value=Percent("trigger_threshold",10); Controls.Add(threshold);

        LabelAt("Anti-Aliasing (AA)",587); aa.SetBounds(292,582,210,28);

        aa.DropDownStyle=ComboBoxStyle.DropDownList; aa.Items.AddRange(new string[]{"Off","FXAA"});

        aa.SelectedIndex=Get("aa","off")=="fxaa"?1:0; Controls.Add(aa);

        LabelAt("Anisotropic Filtering (AF)",623); af.SetBounds(292,618,210,28);

        af.DropDownStyle=ComboBoxStyle.DropDownList; af.Items.AddRange(new string[]{"Off","2x","4x","8x","16x"});

        int afIndex=Array.IndexOf(afValues,Get("anisotropic","16x")); af.SelectedIndex=afIndex<0?4:afIndex; Controls.Add(af);

        fps.Text="Show FPS Counter (F10 toggles in game)";

        fps.SetBounds(16,648,485,26); fps.Checked=Get("fps_counter","on")=="on"; Controls.Add(fps);

        prompts.Text="Button Prompts Follow Xbox Mapping";

        prompts.SetBounds(16,682,485,26); prompts.Checked=Get("xbox_prompts","on")=="on"; Controls.Add(prompts);

        performance.Text="Performance Mode (Faster Game Calculations)";

        performance.SetBounds(16,716,485,26); performance.Checked=Get("vu_fast","off")=="on"; Controls.Add(performance);

        ButtonAt("Original",16,130,delegate { Profile(original,25); });

        ButtonAt("Modern: RT / LT",157,165,delegate { Profile(modern,10); });

        ButtonAt("Save",337,165,Save);

    }

    void Profile(string[] profile,int value) { for(int i=0;i<16;i++) maps[i].Text=profile[i]; threshold.Value=value; }

    void Save(object sender,EventArgs e) {

        try {

            string exe=Path.GetFullPath(Path.Combine(dir,"burnout3.exe"));

            foreach(var p in Process.GetProcessesByName("burnout3")) {

                try { if(string.Equals(p.MainModule.FileName,exe,StringComparison.OrdinalIgnoreCase)) {

                    MessageBox.Show(this,"Close the customized game before saving.","Burnout 3"); return;

                } } catch(System.ComponentModel.Win32Exception) {}

                finally { p.Dispose(); }

            }

            for(int i=0;i<16;i++) {

                string value=maps[i].Text.Trim().ToLowerInvariant();

                if(value!="none") foreach(string token in value.Split('+')) if(Array.IndexOf(original,token)<0) {

                    MessageBox.Show(this,"Invalid button: "+value,"Burnout 3"); return;

                }

                settings["map."+keys[i]]=value;

            }

            settings["rumble_strength"]=rumble.Value.ToString(); settings["trigger_threshold"]=threshold.Value.ToString();

            settings["aa"]=aa.SelectedIndex==1?"fxaa":"off"; settings["anisotropic"]=afValues[af.SelectedIndex];

            settings["fps_counter"]=fps.Checked?"on":"off";

            settings["xbox_prompts"]=prompts.Checked?"on":"off";

            settings["vu_fast"]=performance.Checked?"on":"off";

            string path=Path.Combine(dir,"pc_settings.ini"); var lines=new List<string>();

            foreach(var pair in settings) lines.Add(pair.Key+"="+pair.Value);

            File.WriteAllLines(path+".new",lines.ToArray(),new UTF8Encoding(false));

            if(File.Exists(path)) File.Replace(path+".new",path,path+".controls-backup");

            else File.Move(path+".new",path);

            Close();

        } catch(Exception error) { MessageBox.Show(this,error.Message,"Could Not Save"); }

    }

    [STAThread] static void Main() { Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false); Application.Run(new ControlsForm()); }

}
