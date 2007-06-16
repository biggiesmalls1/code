/*
 * CurrentBotStatePanel.java
 *
 * Created on February 24, 2002, 1:19 AM
 */

package edu.tamu.gamebots.ui;

import edu.isi.gamebots.client.*;

/**
 * <p>
 * A panel for displaying updates of the bots current state.
 * Currently it displays:
 * <ul>
 * <li>Health</li>
 * <li>Armor</li>
 * <li>Location</li>
 * <li>Weapon</li>
 * <li>Ammo</li>
 * <li>Rotation</li>
 * </ul>
 * </p>
 * @author rtr7684
 */
public class CurrentBotStatePanel extends javax.swing.JPanel {

  /** Creates new form CurrentBotStatePanel */
    public CurrentBotStatePanel() {
        initComponents();
    }
    
    /**
     * <p>
     * Updates the display based on a new SLF message.
     * </p>
     * @param selfMessage The SLF message from the UT Gamebots server
     */    
    public void update(Message selfMessage){
      healthField.setText(selfMessage.getProperty("Health"));
      armorField.setText(selfMessage.getProperty("Armor"));
      locationField.setText(selfMessage.getProperty("Location"));
      weaponField.setText(selfMessage.getProperty("Weapon"));
      ammoField.setText(selfMessage.getProperty("CurrentAmmo"));
      rotationField.setText(selfMessage.getProperty("Rotation"));
      velocityField.setText(selfMessage.getProperty("Velocity"));
      revalidate();
    }

    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
  private void initComponents() {//GEN-BEGIN:initComponents
    jPanel1 = new javax.swing.JPanel();
    helathLabel = new javax.swing.JLabel();
    healthField = new javax.swing.JTextField();
    armorLabel = new javax.swing.JLabel();
    armorField = new javax.swing.JTextField();
    weaponLabel = new javax.swing.JLabel();
    weaponField = new javax.swing.JTextField();
    ammoLabel = new javax.swing.JLabel();
    ammoField = new javax.swing.JTextField();
    jPanel2 = new javax.swing.JPanel();
    locationLabel = new javax.swing.JLabel();
    locationField = new javax.swing.JTextField();
    rotationLabel = new javax.swing.JLabel();
    rotationField = new javax.swing.JTextField();
    velocityLabel = new javax.swing.JLabel();
    velocityField = new javax.swing.JTextField();

    setLayout(new javax.swing.BoxLayout(this, javax.swing.BoxLayout.Y_AXIS));

    jPanel1.setLayout(new javax.swing.BoxLayout(jPanel1, javax.swing.BoxLayout.X_AXIS));

    helathLabel.setText("Health: ");
    jPanel1.add(helathLabel);

    healthField.setEditable(false);
    healthField.setBorder(null);
    jPanel1.add(healthField);

    armorLabel.setText("Armor: ");
    jPanel1.add(armorLabel);

    armorField.setEditable(false);
    armorField.setBorder(null);
    jPanel1.add(armorField);

    weaponLabel.setText("Weapon: ");
    jPanel1.add(weaponLabel);

    weaponField.setEditable(false);
    weaponField.setBorder(null);
    jPanel1.add(weaponField);

    ammoLabel.setText("Ammo: ");
    jPanel1.add(ammoLabel);

    ammoField.setEditable(false);
    ammoField.setBorder(null);
    jPanel1.add(ammoField);

    add(jPanel1);

    jPanel2.setLayout(new javax.swing.BoxLayout(jPanel2, javax.swing.BoxLayout.X_AXIS));

    locationLabel.setText("Location: ");
    jPanel2.add(locationLabel);

    locationField.setEditable(false);
    locationField.setBorder(null);
    locationField.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(java.awt.event.ActionEvent evt) {
        locationFieldActionPerformed(evt);
      }
    });

    jPanel2.add(locationField);

    rotationLabel.setText("Rotation: ");
    jPanel2.add(rotationLabel);

    rotationField.setEditable(false);
    rotationField.setBorder(null);
    jPanel2.add(rotationField);

    velocityLabel.setText("Velocity: ");
    jPanel2.add(velocityLabel);

    velocityField.setEditable(false);
    velocityField.setBorder(null);
    jPanel2.add(velocityField);

    add(jPanel2);

  }//GEN-END:initComponents

  private void locationFieldActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_locationFieldActionPerformed
    // Add your handling code here:
  }//GEN-LAST:event_locationFieldActionPerformed


  // Variables declaration - do not modify//GEN-BEGIN:variables
  private javax.swing.JLabel locationLabel;
  private javax.swing.JTextField weaponField;
  private javax.swing.JTextField healthField;
  private javax.swing.JTextField armorField;
  private javax.swing.JPanel jPanel2;
  private javax.swing.JLabel weaponLabel;
  private javax.swing.JPanel jPanel1;
  private javax.swing.JLabel armorLabel;
  private javax.swing.JLabel helathLabel;
  private javax.swing.JTextField velocityField;
  public javax.swing.JTextField rotationField;
  private javax.swing.JLabel velocityLabel;
  private javax.swing.JLabel rotationLabel;
  private javax.swing.JTextField ammoField;
  private javax.swing.JLabel ammoLabel;
  public javax.swing.JTextField locationField;
  // End of variables declaration//GEN-END:variables

}