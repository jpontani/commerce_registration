Commerce Registration
 Allows the sale of registrations via Drupal Commerce.

Required Modules:
 - Drupal Commerce
 - Entity Registrations
 - Views

Supported Modules:
 - Rules

Setup:
 Once all the required modules are enabled, you'll want to start by making sure
 you have a Registration Type created. To do so, go to your Administration area
 and go to Structure -> Registration Types. If you need to create a type, click
 on Add registration type and add any fields you want.
 
 Once you have made sure your registration type is created, go to the admin
 area for your product types under Store -> Products -> Product Types. To add
 the ability to register for a product, you must add a new field to the product
 that is of the type Registration. When you do this, it will add a new field
 to choose the default registration type for new products and when editing a
 product. It will also add a collapsible fieldset for the registration settings
 on the new or existing product.
 
 You must now modify the settings for a particular product, or create a new
 product with registration settings.
 
 Once done with the settings, your product is now available for registration.
 To test, simply add the product to your cart, and proceed with checkout. You
 will see a new checkout page and checkout pane titled Registration Information
 that will have any fields you added to your registration type now displayed
 depending on the quantity of the product you added to your cart.
 
 When through with the order, the registration will be saved once the checkout
 is complete. You can now view registrations for the product by going to the
 product display node and clicking on the Registrations tab.
 